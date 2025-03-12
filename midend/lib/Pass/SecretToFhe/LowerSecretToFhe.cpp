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
    // target.addIllegalDialect<secret::SecretDialect>();
    // target.addIllegalOp<secret::MulOp, secret::MulPlainOp>();
    // target.addIllegalOp<secret::AddOp, secret::AddPlainOp>();
    // target.addIllegalOp<secret::SubOp, secret::SubPlainOp>();
    
    mlir::RewritePatternSet secretPatSet(&getContext());
    secretPatSet.add<ArithBasicPattern<secret::MulOp>, ArithBasicPattern<secret::MulPlainOp>, 
                     ArithBasicPattern<secret::AddOp>, ArithBasicPattern<secret::AddPlainOp>,
                     ArithBasicPattern<secret::SubOp>, ArithBasicPattern<secret::SubPlainOp>,
                     ArithNegPattern>
                     (type_converter, secretPatSet.getContext());
    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(secretPatSet)))) {
        signalPassFailure();
    }
}