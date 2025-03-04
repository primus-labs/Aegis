#include <iostream>
#include <memory>

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/IR/PatternMatch.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "Pass/ArithToSecret/LowerArithToSecret.h"
#include "Common/MetadataMgr.h"

#define DEBUG_TYPE "arith-to-secret"

using namespace mlir;
using namespace aegis;
using namespace secret;


// Helper function to check if a value is encrypted
bool isEncrypted(Value value, llvm::DenseMap<Value, bool> &cache) {
    // Check if the value is already in the cache
    auto it = cache.find(value);
    if (it != cache.end()) {
        return it->second;
    }

    // If the value is a BlockArgument, check its attribute.
    // If the BlockArgument value no metadata, then default param is encrypted.
    if (auto arg = mlir::dyn_cast<BlockArgument>(value)) {
        unsigned index = arg.getArgNumber();
        std::string paramType = "param" + std::to_string(index) + ".type";
        const MetadataMgr &metaMgr = MetadataMgr::getInstance();
        auto attr = metaMgr.getMetadata(paramType);
        if (!attr || !mlir::isa<StringAttr>(attr)) {
            cache[value] = true;
            return true;
        }

        bool result = (mlir::cast<StringAttr>(attr).getValue() == "encrypted");
        cache[value] = result;
        return result;
    }

    // If the value is an operation result, check its defining operation
    if (auto opResult = mlir::dyn_cast<OpResult>(value)) {
        Operation *op = opResult.getDefiningOp();

        // If the value is an constop, then return false.
        if (auto const_op = mlir::dyn_cast<arith::ConstantOp>(op)) {
            cache[value] = false;
            return false;
        }

        // If the operation is a secret operation, the result is encrypted
        // if (isa<SecretDialect>(op->getDialect())) {
        //     cache[value] = true;
        //     return true;
        // }

        // For any operation, check if any of its operands is encrypted
        for (Value operand : op->getOperands()) {
            if (isEncrypted(operand, cache)) {
                cache[value] = true;
                return true;
            }
            else {
                cache[value] = false;
                return false;
            }
        }
    }

    // Default to true if the value is neither a BlockArgument nor an OpResult
    cache[value] = true;
    return true;
}


// Transform arith::SelectOp into secret corresponding op(secret::SelectOp)
// and Convert the data type of input/output of the operators
class ArithSelectPattern final : public OpConversionPattern<arith::SelectOp>
{
protected:
    using OpConversionPattern<arith::SelectOp>::typeConverter;

public:
    using OpConversionPattern<arith::SelectOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(arith::SelectOp op, typename arith::SelectOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destType = typeConverter->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "convert the " <<  op.getType() << " failure.\n");
            return failure();
        }

        Value trueVal = op.getTrueValue();
        Value falseVal = op.getFalseValue();
        Value cond = op.getCondition();
        auto trueDestTy = typeConverter->convertType(trueVal.getType());
        auto falseDestTy = typeConverter->convertType(falseVal.getType());
        auto conDestTy = typeConverter->convertType(cond.getType());
        if (!trueDestTy || !falseDestTy || !conDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "trueDestTy=" << trueDestTy << ",falseDestTy=" << falseDestTy << ",conDestTy=" << conDestTy << "/n");
            return failure();
        }

        auto material_true = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), trueDestTy, trueVal);
        auto material_false = typeConverter->materializeTargetConversion(rewriter, op.getLoc(),falseDestTy, falseVal);
        auto material_cond = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), conDestTy, cond);
        LLVM_DEBUG(llvm::dbgs() << "material_true=" << material_true << "material_false=" << material_false
                                << "material_cond=" << material_cond << "\n");

        rewriter.replaceOpWithNewOp<secret::SelectOp>(op, destType, material_cond, material_true, material_false);
        return success();
    };
};


// Transform arith::AddFOp/MulFOp/SubFOp into corresponding secret ops(SecretMulOp/SecretAddOp/SecretSubOp) 
// and convert the data type of input/output of the ops.
template <typename OpType>
class ArithGeneralPattern final : public OpConversionPattern<OpType>
{
protected:
    using OpConversionPattern<OpType>::typeConverter;

public:
    using OpConversionPattern<OpType>::OpConversionPattern;

    bool isZeroValue(Value v) const {
        if (auto constantOp = v.getDefiningOp<secret::CastOp>().getOperand().getDefiningOp<arith::ConstantOp>())
        {
            if (auto floatAttr = mlir::dyn_cast<FloatAttr>(constantOp.getValue()))
            {
                return floatAttr.getValue().isZero();
            }
            else if (auto intAttr = mlir::dyn_cast<IntegerAttr>(constantOp.getValue()))
            {
                return intAttr.getValue().isZero();
            }
        }
        
        return false;
    }

    bool isOneValue(Value v) const {
        if (auto constantOp = v.getDefiningOp<secret::CastOp>().getOperand().getDefiningOp<arith::ConstantOp>())
        {
            if (auto intAttr = mlir::dyn_cast<IntegerAttr>(constantOp.getValue()))
            {
                return intAttr.getValue().isOne();
            }
        }
        
        return false;
    }

    LogicalResult matchAndRewrite(OpType op, typename OpType::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destType = typeConverter->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op " << op << "\n");
            return failure();
        }

        // Materialize the operands where necessary
        llvm::SmallVector<Value> materialized_ops;
        for (Value o : op.getOperands())
        {
            LLVM_DEBUG(llvm::dbgs() << o << "\n");
            llvm::errs() << o << "\n";
            auto opDestTy = typeConverter->convertType(o.getType());
            if (!opDestTy) {
                LLVM_DEBUG(llvm::dbgs() << "call convertType fail for value " << op << "\n");
                return failure();
            }

            if (o.getType() != opDestTy)
            {
                auto new_operand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), opDestTy, o);
                assert(new_operand && "Type Conversion must be not fail");
                materialized_ops.push_back(new_operand);
                LLVM_DEBUG(llvm::dbgs() << "after call materializeTargetConversion, new ops " << new_operand << "\n");
            }
            else
            {
                materialized_ops.push_back(o);
            }
        }

        // Deal with multiplications
        if (std::is_same<OpType, arith::MulFOp>())
        {
            Value lhs = materialized_ops[0];
            Value rhs = materialized_ops[1];
            if (isOneValue(lhs))
            {
                rewriter.replaceOp(op, rhs);
                if (lhs.use_empty())
                {
                    rewriter.eraseOp(lhs.getDefiningOp());
                }
                return success();
            }
            else if (isOneValue(rhs))
            {
                rewriter.replaceOp(op, lhs);
                if (rhs.use_empty())
                {
                    rewriter.eraseOp(rhs.getDefiningOp());
                }
                return success();
            }
            else
            {
                llvm::DenseMap<Value, bool> cache;
                bool bEncLhs = isEncrypted(lhs, cache);
                bool bEncRhs = isEncrypted(rhs, cache);
                if (bEncLhs && bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::MulOp>(op, TypeRange(destType), materialized_ops);
                }
                else if (bEncLhs && !bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::MulPlainOp>(op, TypeRange(destType), lhs, op.getOperand(1));
                }
                else if (!bEncLhs && bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::MulPlainOp>(op, TypeRange(destType), rhs, op.getOperand(0));
                }

                return success();
            }
        }      
        
        // Deal with additions
        else if (std::is_same<OpType, arith::AddFOp>())
        {
            Value lhs = materialized_ops[0];
            Value rhs = materialized_ops[1];
            if (isZeroValue(lhs))
            {
                rewriter.replaceOp(op, rhs);
                auto srcVal = lhs.getDefiningOp<secret::CastOp>().getOperand();       
                if (lhs.use_empty())
                {
                    rewriter.eraseOp(lhs.getDefiningOp());
                }
                if (srcVal.use_empty())
                {
                    rewriter.eraseOp(srcVal.getDefiningOp());
                }
                return success();
            }
            else if (isZeroValue(rhs))
            {
                rewriter.replaceOp(op, lhs);
                auto srcVal = rhs.getDefiningOp<secret::CastOp>().getOperand();
                if (rhs.use_empty())
                {
                    rewriter.eraseOp(rhs.getDefiningOp());
                }
                if (srcVal.use_empty())
                {
                    rewriter.eraseOp(srcVal.getDefiningOp());
                }
                return success();
            }
            else
            {
                llvm::DenseMap<Value, bool> cache;
                bool bEncLhs = isEncrypted(lhs, cache);
                bool bEncRhs = isEncrypted(rhs, cache);
                if (bEncLhs && bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::AddOp>(op, TypeRange(destType), materialized_ops);
                }
                else if (bEncLhs && !bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::AddPlainOp>(op, TypeRange(destType), lhs, op.getOperand(1));
                }
                else if (!bEncLhs && bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::AddPlainOp>(op, TypeRange(destType), rhs, op.getOperand(0));
                }
                return success();
            }
        }

        // Deal with substractions
        else if (std::is_same<OpType, arith::SubFOp>())
        {
            Value lhs = materialized_ops[0];
            Value rhs = materialized_ops[1];
            if (isZeroValue(rhs))
            {
                rewriter.replaceOp(op, lhs);
                if (rhs.use_empty())
                {
                    rewriter.eraseOp(rhs.getDefiningOp());
                }
                return success();
            }
            else
            {
                llvm::DenseMap<Value, bool> cache;
                bool bEncLhs = isEncrypted(lhs, cache);
                bool bEncRhs = isEncrypted(rhs, cache);
                if (bEncLhs && bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::SubOp>(op, TypeRange(destType), materialized_ops);
                }
                else if (bEncLhs && !bEncRhs) {
                    rewriter.replaceOpWithNewOp<secret::SubPlainOp>(op, TypeRange(destType), lhs, op.getOperand(1));
                }
                else if (!bEncLhs && bEncRhs) {
                    // create negop and then create addplain op.
                    // eg: 3 - encrypt(2) => negop(encrypt(2)) + 3
                    if (isZeroValue(lhs)) {
                        rewriter.replaceOpWithNewOp<secret::NegOp>(op, TypeRange(destType), rhs);
                    }
                    else {
                        auto new_lhs = rewriter.create<secret::NegOp>(op.getLoc(), rhs.getType(), rhs);
                        rewriter.replaceOpWithNewOp<secret::AddPlainOp>(op, TypeRange(destType), new_lhs, op.getOperand(0));
                    }
                }
                
                return success();
            }
        }

        return failure();
    };
};

// Transform arith::CmpFOp into secret::cmpOp, 
// and Convert the data type of input/output of the ops
class ArithCmpPattern final : public OpConversionPattern<arith::CmpFOp>
{
public:
    using OpConversionPattern<arith::CmpFOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(arith::CmpFOp op, typename arith::CmpFOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destType = this->getTypeConverter()->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op " << op << "\n");
            return failure();
        }
        
        // Materialize the operands where necessary
        auto lhs = op.getLhs();
        auto rhs = op.getRhs();
        Value new_lhs, new_rhs;

        // Convert the type of operands(inputs)
        auto lhsOpType = typeConverter->convertType(lhs.getType());
        if (!lhsOpType) {
            LLVM_DEBUG(llvm::dbgs() << "the lhs value " << lhs << " convert type fail.\n");
            return failure();
        }
        if (lhs.getType() != lhsOpType)
        {
            new_lhs = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), lhsOpType, lhs);
            assert(new_lhs && "Type Conversion must be not fail");
        }
        else
        {
            new_lhs = lhs;
        }

        auto rhsOpType = typeConverter->convertType(rhs.getType());
        if (!rhsOpType) {
            LLVM_DEBUG(llvm::dbgs() << "the rhs value " << lhs << " convert type fail.\n");
            return failure();
        }
        if (rhs.getType() != rhsOpType)
        {
            new_rhs = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), rhsOpType, rhs);
            assert(new_rhs && "Type Conversion must be not fail");
        }
        else
        {
            new_rhs = rhs;
        }

        arith::CmpFPredicate predicate = op.getPredicate();
        rewriter.replaceOpWithNewOp<secret::CmpOp>(op, TypeRange(destType), predicate, new_lhs, new_rhs);

        return success();
    }
    
};

void LowerArithToSecretPass::getDependentDialects(mlir::DialectRegistry &registry) const 
{
    registry.insert<arith::ArithDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<func::FuncDialect>();
    registry.insert<secret::SecretDialect>();
    registry.insert<scf::SCFDialect>();
    registry.insert<memref::MemRefDialect>();
}

void LowerArithToSecretPass::collectAllMetadata(mlir::Operation *op) {
    MetadataMgr &metaMgr = MetadataMgr::getInstance();
    op->walk([&](mlir::Operation *op) {
        if (op->getName().getStringRef() == "llvm.metadata") {
            for (auto attr : op->getAttrs()) {
                LLVM_DEBUG(llvm::dbgs() << " collect metadata name: " << attr.getName()
                            << ", value:" << attr.getValue() << "\n");
                metaMgr.addMetadata(attr.getName(), attr.getValue());
            }
        }
    });
}

void LowerArithToSecretPass::runOnOperation() {
    // Collect the module all metadata
    collectAllMetadata(getOperation());

    auto type_converter = TypeConverter();

    // Add type converter to convert plaintext data type to secret & secretvect & secretmatrix type
    type_converter.addConversion([&](Type t) {
        if (mlir::isa<Float32Type>(t))
            return std::optional<Type>(SecretType::get(&getContext(), t));
        else if (mlir::isa<IntegerType>(t))
            return std::optional<Type>(SecretType::get(&getContext(), Float32Type::getF32(&getContext())));
        else if (mlir::isa<MemRefType>(t))
        {
            auto new_t = mlir::cast<MemRefType>(t);
            if (new_t.hasStaticShape() && new_t.getShape().size() == 1) {
                int sizes = new_t.getShape().front();
                return std::optional<Type>(SecretVectorType::get(&getContext(), new_t.getElementType(), sizes));
            }
            else if (new_t.hasStaticShape() && new_t.getShape().size() == 2) {
                int row = new_t.getShape().front();
                int col = new_t.getShape().back();
                return std::optional<Type>(SecretMatrixType::get(&getContext(), new_t.getElementType(), row, col));
            }
            else {
                LLVM_DEBUG(llvm::dbgs() << t << "\n");
                return std::optional<Type>(t);
            }
        }
        else {
            return std::optional<Type>(t);
        }
    });

    type_converter.addTargetMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto ot = mlir::dyn_cast_or_null<SecretType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<Float32Type>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
            else if (mlir::dyn_cast_or_null<IntegerType>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        else if (auto ot = mlir::dyn_cast_or_null<SecretVectorType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        else if (auto ot = mlir::dyn_cast_or_null<SecretMatrixType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto ot = mlir::dyn_cast_or_null<SecretType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<Float32Type>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
            else if (mlir::dyn_cast_or_null<IntegerType>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        else if (auto ot = mlir::dyn_cast_or_null<SecretVectorType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        else if (auto ot = mlir::dyn_cast_or_null<SecretMatrixType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto bst = mlir::dyn_cast_or_null<Float32Type>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto old_type = vs.front().getType();
            if (auto ot = mlir::dyn_cast_or_null<SecretType>(old_type))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, bst, vs));
            }
        }
        else if (auto bst = mlir::dyn_cast_or_null<IntegerType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto old_type = vs.front().getType();
            if (auto ot = mlir::dyn_cast_or_null<SecretType>(old_type))
                return std::optional<Value>(builder.create<secret::CastOp>(loc, bst, vs));
        }
        else if (auto bst = mlir::dyn_cast_or_null<MemRefType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto old_type = vs.front().getType();
            if (auto ot = mlir::dyn_cast_or_null<SecretVectorType>(old_type))
                return std::optional<Value>(builder.create<secret::CastOp>(loc, bst, vs));
            else if (auto ot = mlir::dyn_cast_or_null<SecretMatrixType>(old_type))
                return std::optional<Value>(builder.create<secret::CastOp>(loc, bst, vs));
        }
        return std::optional<Value>(std::nullopt);
    });


    ConversionTarget target(getContext());
    IRRewriter rewriter(&getContext());
    target.addLegalDialect<affine::AffineDialect, func::FuncDialect, scf::SCFDialect, arith::ArithDialect>();
    target.addLegalDialect<memref::MemRefDialect>();
    target.addLegalDialect<SecretDialect>();
    target.addLegalOp<ModuleOp>();
    target.addIllegalOp<arith::MulFOp>();
    target.addIllegalOp<arith::AddFOp>();
    target.addIllegalOp<arith::SubFOp>();
    target.addIllegalOp<arith::CmpFOp>();
    target.addIllegalOp<memref::AllocaOp>();

    //Convert arith::select to secret::select.
    mlir::RewritePatternSet selectPatSet(&getContext());
    ConversionTarget target_2(getContext());
    target_2.addLegalDialect<SecretDialect>();
    target_2.addLegalDialect<affine::AffineDialect, func::FuncDialect, scf::SCFDialect, arith::ArithDialect>();
    target_2.addLegalDialect<memref::MemRefDialect>();
    target_2.addLegalOp<ModuleOp>();
    target_2.addIllegalOp<arith::SelectOp>();

    selectPatSet.add<ArithSelectPattern>(type_converter, selectPatSet.getContext()); 
    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target_2, std::move(selectPatSet)))) {
        LLVM_DEBUG(llvm::dbgs() << "apply ArithSelectPattern fail.\n");
        signalPassFailure();
    }
    
    // Convert arith::mulf,addf,subf... to secret::mul,addf,subf...
    mlir::RewritePatternSet arithPatSet(&getContext());
    arithPatSet.add<ArithGeneralPattern<arith::MulFOp>, ArithGeneralPattern<arith::AddFOp>, ArithGeneralPattern<arith::SubFOp>,
                    ArithCmpPattern> (type_converter, arithPatSet.getContext());
    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(arithPatSet)))) {
        LLVM_DEBUG(llvm::dbgs() << "apply arithtPatternSet(mulf,addf,subf...) fail.\n");
        signalPassFailure();
    }
}
