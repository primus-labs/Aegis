#include <memory>
#include <iostream>

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "Pass/FuncToSecret/LowerFuncToSecret.h"
#include "Common/MetadataMgr.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "func-to-secret"

using namespace mlir;
using namespace aegis;
using namespace secret;



// Transform func::CallOp to secret::CallOp and 
// convert arguments types into secret types and 
class FuncCallPattern final : public OpConversionPattern<func::CallOp>
{
protected:
    using OpConversionPattern<func::CallOp>::typeConverter;

public:
    using OpConversionPattern<func::CallOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::CallOp op, typename func::CallOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        llvm::SmallVector<Value> materialize_ops;
        for (Value o : op.getOperands())
        {
            auto opDestType = typeConverter->convertType(o.getType());
            if (!opDestType) {
                LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op( " << o << " )\n");
                return failure();
            }

            if (o.getType() != opDestType) {
                auto new_operand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), opDestType, o);
                assert(new_operand && "Type Conversion must be not fail");
                materialize_ops.push_back(new_operand);
            }
            else {
                materialize_ops.push_back(o);
            }
        }

        // Get function name
        auto func_name = op.getCallee();

        // Only support one result
        if (op.getNumResults() > 1) {
            LLVM_DEBUG(llvm::dbgs() << "only support one result now.\n");
            emitError(op.getLoc(), "Currently only single result value are supported.");
            return failure();
        }

        if (op.getNumResults() == 1) {
            auto resType = getTypeConverter()->convertType(op.getResult(0).getType());
            if (!resType) {
                LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op type( " << op.getResult(0) << " )\n");
                return failure();
            }
        
            //rewriter.replaceOpWithNewOp<Secret::CallOp>(op, TypeRange(resType), func_name, ArrayAttr(), ArrayAttr(), materialize_ops);

        } 
        else {
            return failure();
        }
        
        return success();
    }
};

// Convert all numeric function arguments in a function block to secret types.
class FunctionPattern final : public OpConversionPattern<func::FuncOp>
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
            
            if (isArgEncrypted(arg)) {
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
            if (!isArgEncrypted(arg)) {
                //skip the clear argument.
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
        // op.print(llvm::errs());

        return success();
    }
};

// Convert the type of return value in a function block to secret types.
class ReturnPattern final : public OpConversionPattern<func::ReturnOp>
{
public:
    using OpConversionPattern<func::ReturnOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::ReturnOp op, typename func::ReturnOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        if (op.getNumOperands() != 1)
        {
            emitError(op.getLoc(), "Currently only single value return operations are supported.");
            return failure();
        }

        auto destTy = this->getTypeConverter()->convertType(op.getOperandTypes().front());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for type( " << op.getOperandTypes().front() << " )\n");
            return failure();
        }

        rewriter.setInsertionPoint(op);
        Value retVal;
        if (auto dt = mlir::dyn_cast_or_null<secret::SecretVectorType>(destTy)){
            retVal = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), destTy, op.getOperands());
        }
        else {
            retVal = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), destTy, op.getOperands());
        }
        rewriter.replaceOpWithNewOp<func::ReturnOp>(op, retVal);

        return success();
    }
};


void LowerFuncToSecretPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<func::FuncDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<arith::ArithDialect>();
    registry.insert<secret::SecretDialect>();
}

void LowerFuncToSecretPass::runOnOperation() {
    auto type_converter = TypeConverter();

    // Add type converter to convert numeric data type to secret type
    type_converter.addConversion([&](Type t) {
        if (mlir::isa<FloatType>(t)) {
            return std::optional<Type>(SecretType::get(&getContext(), t));
        }
        else if (mlir::isa<MemRefType>(t)) {
            auto newTy = mlir::cast<MemRefType>(t);
            if (newTy.hasStaticShape() && newTy.getShape().size() == 1) {
                int size = newTy.getShape().front();
                return std::optional<Type>(SecretVectorType::get(&getContext(), newTy.getElementType(), size));
            }
            else if (newTy.hasStaticShape() && newTy.getShape().size() == 2) {
                auto row = newTy.getShape().front();
                auto col = newTy.getShape().back();
                return std::optional<Type>(SecretMatrixType::get(&getContext(), newTy.getElementType(), row, col));
            }
            else {
                return std::optional<Type>(t);
            }
        }
        else {
            return std::optional<Type>(t);
        }
    });

    type_converter.addTargetMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<SecretType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<FloatType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
            else if (mlir::dyn_cast_or_null<IntegerType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<SecretVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<SecretMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }

        LLVM_DEBUG(llvm::dbgs() << "call addTargetMaterialization failure, return null type.(at LowerFuncToSecret Pass)\n");
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<SecretType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<FloatType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<SecretVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<SecretMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }

        LLVM_DEBUG(llvm::dbgs() << "call addArgumentMaterialization failure, return null type.(at LowerFuncToSecret Pass)\n");
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<FloatType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<SecretType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<MemRefType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (auto _ = mlir::dyn_cast_or_null<SecretVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
            else if (auto _ = mlir::dyn_cast_or_null<SecretMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }

        LLVM_DEBUG(llvm::dbgs() << "call addSourceMaterialization failure, return null type.(at LowerFuncToSecret Pass)\n");
        return std::optional<Value>(std::nullopt);
    });
    
    ConversionTarget target(getContext());
    target.addLegalDialect<affine::AffineDialect, func::FuncDialect, scf::SCFDialect, arith::ArithDialect>();
    target.addLegalDialect<secret::SecretDialect>();
    target.addLegalOp<ModuleOp>();
    // target.addLegalDialect<memref::MemRefDialect>();
    target.addLegalOp<memref::GlobalOp>();
    target.addLegalOp<memref::GetGlobalOp>();
    target.addLegalOp<memref::LoadOp>();
    target.addLegalOp<memref::StoreOp>();
    target.addLegalOp<memref::AllocOp>();
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

        // Since function parameters may be marked as built-in types based on metadata, 
        // their legality is not checked here.
        // for (auto t : fop.getFunctionType().getInputs()) {
        //     if (!type_converter.isLegal(t))
        //         return false;
        // }
        for (auto t : fop.getFunctionType().getResults()) {
            if (!type_converter.isLegal(t))
                return false;
        }
        return true;
    });
    
    target.addDynamicallyLegalOp<func::ReturnOp>([&](Operation *op) { 
        return type_converter.isLegal(op->getOperandTypes()); 
    });

    IRRewriter rewriter(&getContext());
    mlir::RewritePatternSet funcPatSet(&getContext());
    funcPatSet.add<FunctionPattern, ReturnPattern, FuncCallPattern>(type_converter, funcPatSet.getContext()); 
    if (mlir::failed(mlir::applyFullConversion(getOperation(), target, std::move(funcPatSet))))
        signalPassFailure();
}