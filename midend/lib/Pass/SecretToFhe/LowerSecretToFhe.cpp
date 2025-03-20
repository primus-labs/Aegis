#include <iostream>
#include <memory>

#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
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
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/SecretToFhe/LowerSecretToFhe.h"
#include "Common/MetadataMgr.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "secret-to-fhe"

using namespace mlir;
using namespace aegis;
// using namespace mlir::aegis::secret;
// using namespace mlir::aegis::fhe;


class ArithNegPattern final : public OpConversionPattern<secret::NegOp> 
{
protected:
    using OpConversionPattern<secret::NegOp>::typeConverter;

public:
    using OpConversionPattern<secret::NegOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::NegOp op, typename secret::NegOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destType = typeConverter->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op " << op << "\n");
            return failure();
        }
 
        Value opVal = op.getOperand();
        auto opDestTy = typeConverter->convertType(opVal.getType());
        if (!opDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for value " << opVal << "\n");
            return failure();
        }

        Value newOpVal = opVal;
        if (opVal.getType() != opDestTy)
        {
            auto new_operand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), opDestTy, opVal);
            assert(new_operand && "Type Conversion must be not fail");
            newOpVal = new_operand;
            LLVM_DEBUG(llvm::dbgs() << "after call materializeTargetConversion, new ops " << new_operand << "\n");
        }

        rewriter.replaceOpWithNewOp<fhe::LWENegOp>(op, destType, newOpVal);
        return success();
    }
};


// Transform secret::AddOp/AddPlainOp/MulOp/MulPlain/SubOp/SubPlain into corresponding fhe ops(FHEMulOp/FHEAddOp/FHESubOp...) 
// and convert the data type of input/output of the ops.
template <typename OpType>
class ArithBasicPattern final : public OpConversionPattern<OpType>
{
protected:
    using OpConversionPattern<OpType>::typeConverter;

public:
    using OpConversionPattern<OpType>::OpConversionPattern;

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
        if (std::is_same<OpType, secret::MulOp>()) {
            rewriter.replaceOpWithNewOp<fhe::LWEMulOp>(op, TypeRange(destType), materialized_ops);
            return success();
        }
        else if (std::is_same<OpType, secret::MulPlainOp>()) {
            llvm::SmallVector<Value> params;
            params.push_back(materialized_ops[0]);
            params.push_back(op.getOperand(1));
            rewriter.replaceOpWithNewOp<fhe::LWEMulPlainOp>(op, TypeRange(destType), params);
            return success();
        }
        // Deal with additions
        else if (std::is_same<OpType, secret::AddOp>()) {
            rewriter.replaceOpWithNewOp<fhe::LWEAddOp>(op, TypeRange(destType), materialized_ops);
            return success();
        }
        else if (std::is_same<OpType, secret::AddPlainOp>()) {
            llvm::SmallVector<Value> params;
            params.push_back(materialized_ops[0]);
            params.push_back(op.getOperand(1));
            rewriter.replaceOpWithNewOp<fhe::LWEAddPlainOp>(op, TypeRange(destType), params);
            return success();
        }
        // Deal with substractions
        else if (std::is_same<OpType, secret::SubOp>()) {
            rewriter.replaceOpWithNewOp<fhe::LWESubOp>(op, TypeRange(destType), materialized_ops);
            return success();
        }
        else if (std::is_same<OpType, secret::SubPlainOp>()) {
            llvm::SmallVector<Value> params;
            params.push_back(materialized_ops[0]);
            params.push_back(op.getOperand(1));
            rewriter.replaceOpWithNewOp<fhe::LWESubPlainOp>(op, TypeRange(destType), params);
            return success();
        }

        // LLVM_DEBUG(llvm::dbgs() << "run SecretBasicPattern failure, unkown the op " << op << "\n");
        llvm::outs() << "run SecretBasicPattern failure, unkown the op " << op << "\n";
        return failure();
    };
};


// Transform secret::CmpFOp into fhe::cmpOp, and Convert the data type of input/output of the ops
class SecretCmpPattern final : public OpConversionPattern<secret::CmpOp>
{
public:
    using OpConversionPattern<secret::CmpOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::CmpOp op, typename secret::CmpOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
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
        if (lhs.getType() != lhsOpType) {
            new_lhs = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), lhsOpType, lhs);
            assert(new_lhs && "Type Conversion must be not fail");
        }
        else {
            new_lhs = lhs;
        }

        auto rhsOpType = typeConverter->convertType(rhs.getType());
        if (!rhsOpType) {
            LLVM_DEBUG(llvm::dbgs() << "the rhs value " << lhs << " convert type fail.\n");
            return failure();
        }
        if (rhs.getType() != rhsOpType) {
            new_rhs = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), rhsOpType, rhs);
            assert(new_rhs && "Type Conversion must be not fail");
        }
        else {
            new_rhs = rhs;
        }

        arith::CmpFPredicate predicate = op.getPredicate();
        rewriter.replaceOpWithNewOp<fhe::CmpOp>(op, TypeRange(destType), predicate, new_lhs, new_rhs);

        return success();
    }
};


// Transform secret::SelectOp into secret corresponding op(fhe::SelectOp)
class SecretSelectPattern final : public OpConversionPattern<secret::SelectOp>
{
protected:
    using OpConversionPattern<secret::SelectOp>::typeConverter;

public:
    using OpConversionPattern<secret::SelectOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::SelectOp op, typename secret::SelectOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
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

        rewriter.replaceOpWithNewOp<fhe::SelectOp>(op, destType, material_cond, material_true, material_false);
        return success();
    };
};


// Convert all secret function arguments in a function block to fhe types.
class SecretFuncPattern final : public OpConversionPattern<func::FuncOp>
{
public:
    using OpConversionPattern<func::FuncOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::FuncOp op, typename func::FuncOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        // Generate the new signature of the function.
        SmallVector<Type> newResTypes;
        if (failed(typeConverter->convertTypes(op.getFunctionType().getResults(), newResTypes))) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for function result op type( " << op.getFunctionType().getResults() << " )\n");
            return failure();
        }

        // Iterate through all parameters and process them one by one.
        TypeConverter::SignatureConversion signatureConversion(op.getFunctionType().getNumInputs());
        for (auto [index, arg] : llvm::enumerate(op.getRegion().getArguments())) {
            Type originalType = op.getFunctionType().getInput(index);
            
            if (mlir::isa<secret::SecretType>(originalType) ||
                mlir::isa<secret::SecretVectorType>(originalType) ||
                mlir::isa<secret::SecretMatrixType>(originalType)) {
                SmallVector<Type> destTypes;
                if (failed(typeConverter->convertType(originalType, destTypes))) {
                    LLVM_DEBUG(llvm::dbgs() << "all convertType fail for type("  << originalType << " )\n");
                    return failure();
                }
                signatureConversion.addInputs(index, destTypes);
            } 
            else {
                signatureConversion.addInputs(index, {originalType});
            }
        }

        auto newFuncTy = FunctionType::get(getContext(), signatureConversion.getConvertedTypes(), newResTypes);
        rewriter.startOpModification(op);
        op.setType(newFuncTy);
        for (BlockArgument arg : op.getRegion().getArguments()) {
            if (!(mlir::isa<secret::SecretType>(arg.getType()) ||
                  mlir::isa<secret::SecretVectorType>(arg.getType()) ||
                  mlir::isa<secret::SecretMatrixType>(arg.getType()))) {
                continue;
            }

            auto oldType = arg.getType();
            auto newType = typeConverter->convertType(oldType);
            if (!newType) {
                LLVM_DEBUG(llvm::dbgs() << "call convertType fail for type( " << oldType << " )\n");
                return failure();
            }

            arg.setType(newType);
            if (newType != oldType)
            {
                rewriter.setInsertionPointToStart(&op.getBody().getBlocks().front());
                auto cast_op = typeConverter->materializeSourceConversion(rewriter, arg.getLoc(), oldType, arg);
                arg.replaceAllUsesExcept(cast_op, cast_op.getDefiningOp());      
            }
        }
        rewriter.finalizeOpModification(op);
        // op.print(llvm::outs());

        return success();
    }
};


// Convert secret type of return value in a function block to fhe types.
class SecretRetPattern final : public OpConversionPattern<func::ReturnOp>
{
public:
    using OpConversionPattern<func::ReturnOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::ReturnOp op, typename func::ReturnOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        if (op.getNumOperands() != 1) {
            emitError(op.getLoc(), "Currently only single value return operations are supported.");
            return failure();
        }

        auto destTy = this->getTypeConverter()->convertType(op.getOperandTypes().front());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for type( " << op.getOperandTypes().front() << " )\n");
            return failure();
        }

        rewriter.setInsertionPoint(op);
        Value retVal = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), destTy, op.getOperands());
        assert(retVal);
        rewriter.replaceOpWithNewOp<func::ReturnOp>(op, retVal);

        return success();
    }
};


// Transform the secret::LoadOp to fhe::LoadOp, convert secret type to LWECipher type.
class SecretLoadPattern final : public OpConversionPattern<secret::LoadOp> {
public:
    using OpConversionPattern<secret::LoadOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::LoadOp op, typename secret::LoadOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override {
        auto srcTy = op.getMemref().getType();
        if (!(mlir::isa<secret::SecretType>(srcTy) ||
              mlir::isa<secret::SecretVectorType>(srcTy) ||
              mlir::isa<secret::SecretMatrixType>(srcTy))) {
            return success();
        }

        auto destTy = this->getTypeConverter()->convertType(srcTy);
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "convert type " << srcTy << " failure.\n");
            return failure();
        }
        auto fheVal = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), destTy, op.getMemref());

        // Get lwecipher Plaintext Type
        mlir::Type destUnitTy;
        if (auto CipherTy = mlir::dyn_cast_or_null<fhe::LWECipherType>(destTy)) {
            destUnitTy = CipherTy.getPlaintextType();
        }
        else if (auto CipherTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(destTy)) {
            destUnitTy = CipherTy.getPlaintextType();
        }
        else if (auto CipherTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(destTy)) {
            destUnitTy = CipherTy.getPlaintextType();
        }

        SmallVector<Value, 8> indices(adaptor.getIndices());
        auto unitCipherTy = fhe::LWECipherType::get(getContext(), destUnitTy);
        rewriter.replaceOpWithNewOp<fhe::LoadOp>(op, unitCipherTy, fheVal, indices);
        
        LLVM_DEBUG(llvm::dbgs() << "run SecretLoadPattern success.\n");
        return success();
    }
};


// Transform secret::StoreOp to fhe::StoreOp, convert secret type to LWECipher type.
class SecretStorePattern final : public OpConversionPattern<secret::StoreOp> {
public:
    using OpConversionPattern<secret::StoreOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::StoreOp op, typename secret::StoreOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto srcTy = op.getMemref().getType();
        if (!(mlir::isa<secret::SecretType>(srcTy) ||
              mlir::isa<secret::SecretVectorType>(srcTy) ||
              mlir::isa<secret::SecretMatrixType>(srcTy))) {
            return success();
        }

        auto destTy = this->getTypeConverter()->convertType(srcTy);
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "convert memref type " << srcTy << " failure.\n");
            return failure();
        }
        auto valueToStoreDestTy = this->getTypeConverter()->convertType(op.getValueToStore().getType());
        if (!valueToStoreDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "convert ValueToStore type " << op.getValueToStore().getType() << " failure.\n");
            return failure();
        }

        auto fheArrVal = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), destTy, op.getMemref());
        auto fheValToStore = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), valueToStoreDestTy, op.getValueToStore());
        SmallVector<Value, 8> indices(adaptor.getIndices());
        rewriter.replaceOpWithNewOp<fhe::StoreOp>(op, fheValToStore, fheArrVal, indices);
        
        LLVM_DEBUG(llvm::dbgs() << "run SecretStorePattern success.\n");
        return success();
    }
};


// Transform secret::CopyOp to fhe::CopyOp, convert secret type to LWECipher type.
class SecretCopyPattern final : public OpConversionPattern<secret::CopyOp> {
public:
    using OpConversionPattern<secret::CopyOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::CopyOp op, typename secret::CopyOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto srcTy = op.getSource().getType();
        if (!(mlir::isa<secret::SecretType>(srcTy) ||
              mlir::isa<secret::SecretVectorType>(srcTy) ||
              mlir::isa<secret::SecretMatrixType>(srcTy))) {
            return success();
        }

        auto newSrcTy = this->getTypeConverter()->convertType(srcTy);
        if (!newSrcTy) {
            LLVM_DEBUG(llvm::dbgs() << "convert secret::copyop source type " << srcTy << " failure.\n");
            return failure();
        }

        auto newDestTy = this->getTypeConverter()->convertType(op.getTarget().getType());
        if (!newDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "convert secret::copyop target type " << op.getTarget().getType() << " failure.\n");
            return failure();
        }

        auto newSrcVal  = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), newSrcTy, op.getSource());
        auto newDestVal = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), newDestTy, op.getTarget());
        rewriter.replaceOpWithNewOp<fhe::CopyOp>(op, newSrcVal, newDestVal);
        
        LLVM_DEBUG(llvm::dbgs() << "run SecretCopyPattern success.\n");
        return success();
    }
};


// Transform secret::AllocaOp to fhe::AllocaOp.
class SecretAllocaPattern final : public OpConversionPattern<secret::AllocaOp>
{
public:
    using OpConversionPattern<secret::AllocaOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::AllocaOp op, typename secret::AllocaOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destType = this->getTypeConverter()->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "convert type " << op.getType() << " failure.\n");
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<fhe::AllocaOp>(op, destType);

        LLVM_DEBUG(llvm::dbgs() << "run SecretAllocaPattern success.\n");
        return success();
    }
};


// Transform secret::AllocOp to secret::AllocOp.
class SecretAllocPattern final : public OpConversionPattern<secret::AllocOp>
{
public:
    using OpConversionPattern<secret::AllocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::AllocOp op, typename secret::AllocOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destType = this->getTypeConverter()->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "convert type " << op.getType() << " failure.\n");
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<fhe::AllocOp>(op, destType);

        LLVM_DEBUG(llvm::dbgs() << "run SecretAllocPattern success.\n");
        return success();
    }
};


// Transform secret::DeallocOp to fhe::DeallocOp.
class SecretDeallocPattern final : public OpConversionPattern<secret::DeallocOp>
{
public:
    using OpConversionPattern<secret::DeallocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(secret::DeallocOp op, typename secret::DeallocOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        Value newOperand;
        auto o = op.getOperand();
        auto opDestTy = typeConverter->convertType(o.getType());
        if (!opDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for value " << op << "\n");
            return failure();
        }

        if (o.getType() != opDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), opDestTy, o);
            assert(newOperand && "Type Conversion must be not fail");
            LLVM_DEBUG(llvm::dbgs() << "after call materializeTargetConversion, new ops " << newOperand << "\n");
        }
        else {
            newOperand = o;
        }
        
        rewriter.replaceOpWithNewOp<fhe::DeallocOp>(op, newOperand);

        LLVM_DEBUG(llvm::dbgs() << "run SecretDeallocPattern success.\n");
        return success();
    }
};


// Transform secret::RevealOp to fhe::RevealOp.
class SecretRevealPattern final : public OpConversionPattern<secret::RevealOp>
{
public:
    using OpConversionPattern<secret::RevealOp>::OpConversionPattern;

    mlir::Type getPlaintextType(Type t) const {
        if (mlir::isa<fhe::LWECipherType>(t)) {
            auto realTy = mlir::cast<fhe::LWECipherType>(t);
            return realTy.getPlaintextType();
        }
        else if (mlir::isa<fhe::LWECipherVectorType>(t)) {
            auto realTy = mlir::cast<fhe::LWECipherVectorType>(t);
            return realTy.getPlaintextType();
        }
        else if (mlir::isa<fhe::LWECipherMatrixType>(t)) {
            auto realTy = mlir::cast<fhe::LWECipherMatrixType>(t);
            return realTy.getPlaintextType();
        }
        else {
            assert(false && "getPlaintextType faiulre, maybe source type incorrect.");
            return mlir::Float32Type::getF32(getContext());
        }
    }

    LogicalResult matchAndRewrite(secret::RevealOp op, typename secret::RevealOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        Value newOperand;
        auto o = op.getOperand();
        auto opDestTy = typeConverter->convertType(o.getType());
        if (!opDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for value " << op << "\n");
            return failure();
        }

        if (o.getType() != opDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), opDestTy, o);
            assert(newOperand && "Type Conversion must be not fail");
            LLVM_DEBUG(llvm::dbgs() << "after call materializeTargetConversion, new ops " << newOperand << "\n");
        }
        else {
            newOperand = o;
        }
        
        auto resTy = getPlaintextType(opDestTy);
        rewriter.replaceOpWithNewOp<fhe::RevealOp>(op, resTy, newOperand);

        LLVM_DEBUG(llvm::dbgs() << "run SecretRevealPattern success.\n");
        return success();
    }
};


void LowerSecretToFhePass::getDependentDialects(mlir::DialectRegistry &registry) const 
{
    registry.insert<func::FuncDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<scf::SCFDialect>();
    registry.insert<arith::ArithDialect>();
    registry.insert<memref::MemRefDialect>();
    registry.insert<secret::SecretDialect>();
    registry.insert<fhe::FHEDialect>();
}

void LowerSecretToFhePass::runOnOperation() {
    auto type_converter = TypeConverter();

    // Add type converter to convert plaintext data type to cipher & ciphervector & ciphermatirx type
    type_converter.addConversion([&](Type t) {
        if (mlir::isa<FloatType>(t)) {
            return std::optional<Type>(fhe::LWECipherType::get(&getContext(), t));
        }
        else if (mlir::isa<IntegerType>(t)) {
            return std::optional<Type>(fhe::LWECipherType::get(&getContext(), Float32Type::getF32(&getContext())));
        }
        else if (mlir::isa<MemRefType>(t)) {
            auto srcTy = mlir::cast<MemRefType>(t);
            if (srcTy.hasStaticShape() && srcTy.getShape().size() == 1) {
                int sizes = srcTy.getShape().front();
                return std::optional<Type>(fhe::LWECipherVectorType::get(&getContext(), srcTy.getElementType(), sizes));
            }
            else if (srcTy.hasStaticShape() && srcTy.getShape().size() == 2) {
                int row = srcTy.getShape().front();
                int col = srcTy.getShape().back();
                return std::optional<Type>(fhe::LWECipherMatrixType::get(&getContext(), srcTy.getElementType(), row, col));
            }
            else {
                LLVM_DEBUG(llvm::dbgs() << t << "\n");
                return std::optional<Type>(t);
            }
        }
        else if (mlir::isa<secret::SecretType>(t)) {
            auto srcTy = mlir::cast<secret::SecretType>(t);
            return std::optional<Type>(fhe::LWECipherType::get(&getContext(), srcTy.getPlaintextType()));
        }
        else if (mlir::isa<secret::SecretVectorType>(t)) {
            auto srcTy = mlir::cast<secret::SecretVectorType>(t);
            return std::optional<Type>(fhe::LWECipherVectorType::get(&getContext(), srcTy.getPlaintextType(), srcTy.getSize()));
        }
        else if (mlir::isa<secret::SecretMatrixType>(t)) {
            auto srcTy = mlir::cast<secret::SecretMatrixType>(t);
            return std::optional<Type>(fhe::LWECipherMatrixType::get(&getContext(), srcTy.getPlaintextType(), srcTy.getRow(), srcTy.getCol()));
        }
        else {
            return std::optional<Type>(t);
        }
    });

    type_converter.addTargetMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<FloatType>(srcTy) ||
                mlir::dyn_cast_or_null<IntegerType>(srcTy) ||
                mlir::dyn_cast_or_null<secret::SecretType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            llvm::outs() << "Don't handle this type:(" << srcTy << ").\n";
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy) ||
                mlir::dyn_cast_or_null<secret::SecretVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            llvm::outs() << "Don't handle this type:(" << srcTy << ").\n";
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy) ||
                mlir::dyn_cast_or_null<secret::SecretMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            llvm::outs() << "Don't handle this type:(" << srcTy << ").\n";
        }

        LLVM_DEBUG(llvm::dbgs() << "call addTargetMaterialization failure, return null type.(at SecretToFhePass)\n");
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<FloatType>(srcTy) ||
                mlir::dyn_cast_or_null<IntegerType>(srcTy) ||
                mlir::dyn_cast_or_null<secret::SecretType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            llvm::outs() << "Don't handle this type:(" << srcTy << ").\n";
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy) ||
                mlir::dyn_cast_or_null<secret::SecretVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            llvm::outs() << "Don't handle this type:(" << srcTy << ").\n";
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy) ||
                mlir::dyn_cast_or_null<secret::SecretMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            llvm::outs() << "Don't handle this type:(" << srcTy << ").\n";
        }

        LLVM_DEBUG(llvm::dbgs() << "call addArgumentMaterialization failure, return null type.(at SecretToFhePass)\n");
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<FloatType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<fhe::LWECipherType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<IntegerType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<fhe::LWECipherType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<MemRefType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            else if (auto _ = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<secret::SecretType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<fhe::LWECipherType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<secret::SecretVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<secret::SecretMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
        }

        llvm::outs() << "Don't handle source type:(" << t << ").\n";
        LLVM_DEBUG(llvm::dbgs() << "call addArgumentMaterialization failure, return null type.(at SecretToFhePass)\n");
        return std::optional<Value>(std::nullopt);
    });


    // Convert secret::mul,add,sub... to fhe::mul,add,sub...
    ConversionTarget target(getContext());
    target.addLegalDialect<affine::AffineDialect, func::FuncDialect, scf::SCFDialect, 
                           arith::ArithDialect, memref::MemRefDialect>();
    target.addLegalDialect<fhe::FHEDialect>();
    target.addLegalOp<ModuleOp>();
    target.addIllegalOp<secret::MulOp, secret::MulPlainOp>();
    target.addIllegalOp<secret::AddOp, secret::AddPlainOp>();
    target.addIllegalOp<secret::SubOp, secret::SubPlainOp, secret::NegOp>();
    target.addIllegalOp<secret::LoadOp, secret::StoreOp, secret::CopyOp>();
    target.addIllegalOp<secret::CmpOp, secret::SelectOp>();
    target.addIllegalOp<secret::AllocaOp, secret::AllocOp, secret::DeallocOp>();
    target.addIllegalOp<secret::RevealOp>();
    target.addIllegalOp<func::CallOp>();
    target.addDynamicallyLegalOp<func::FuncOp>([&](Operation *op) {
        auto fop = llvm::dyn_cast<func::FuncOp>(op);
        for (auto t : op->getOperandTypes()) {
            if (!type_converter.isLegal(t))
                return false;
        }
        for (auto t : op->getResultTypes()) {
            if (!type_converter.isLegal(t))
                return false;
        }
        for (auto t : fop.getFunctionType().getResults()) {
            if (!type_converter.isLegal(t))
                return false;
        }
        return true;
    });  
    target.addDynamicallyLegalOp<func::ReturnOp>([&](Operation *op) { 
        return type_converter.isLegal(op->getOperandTypes()); 
    });
    
    mlir::RewritePatternSet secretPatSet(&getContext());
    secretPatSet.add<ArithBasicPattern<secret::MulOp>, ArithBasicPattern<secret::MulPlainOp>, 
                     ArithBasicPattern<secret::AddOp>, ArithBasicPattern<secret::AddPlainOp>,
                     ArithBasicPattern<secret::SubOp>, ArithBasicPattern<secret::SubPlainOp>,
                     ArithNegPattern,
                     SecretCmpPattern, SecretSelectPattern,
                     SecretFuncPattern, SecretRetPattern,
                     SecretLoadPattern, SecretStorePattern, SecretCopyPattern,
                     SecretAllocaPattern, SecretAllocPattern, SecretDeallocPattern,
                     SecretRevealPattern>
                     (type_converter, secretPatSet.getContext());
    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(secretPatSet)))) {
        signalPassFailure();
    }
}