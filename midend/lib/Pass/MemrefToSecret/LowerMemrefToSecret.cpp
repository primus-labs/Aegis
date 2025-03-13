#include <iostream>
#include <memory>

#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "Pass/MemrefToSecret/LowerMemrefToSecret.h"
#include "Common/MetadataMgr.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "memref-to-secret"

using namespace mlir;
using namespace aegis;
using namespace secret;


// Transform the memref::LoadOp to secret::LoadOp, convert memref type to secret type.
class MemrefLoadPattern final : public OpConversionPattern<memref::LoadOp> {
public:
    using OpConversionPattern<memref::LoadOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::LoadOp op, typename memref::LoadOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override {
        llvm::DenseMap<Value, bool> cache;
        if (!isEncrypted(op.getMemRef(), cache)) {
            return success();
        }

        auto destTy = this->getTypeConverter()->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "convert type " << op.getType() << " failure.\n");
            return failure();
        }

        if (auto st = mlir::dyn_cast_or_null<FloatType>(destTy)) {
            auto memrefTy = mlir::dyn_cast<MemRefType>(op.getMemRef().getType());
            if (!memrefTy || !memrefTy.hasStaticShape()) {
                LLVM_DEBUG(llvm::dbgs() << "memrefTy:" << memrefTy << ", has static shape:" << memrefTy.hasStaticShape());
                return failure();
            }

            Value secretVal;
            if (memrefTy.getShape().size() == 1) {
                int sizes = memrefTy.getShape().front();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                        SecretVectorType::get(getContext(), st, sizes), op.getMemRef());
            }
            else if (memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                        SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
            } 
            else {
                LLVM_DEBUG(llvm::dbgs() << "unsupport shape for " << memrefTy.getShape().size() << ".\n");
                return failure();
            }

            SmallVector<Value, 8> indices(adaptor.getIndices());
            rewriter.replaceOpWithNewOp<secret::LoadOp>(op, destTy, secretVal, indices);
            LLVM_DEBUG(llvm::dbgs() << "run MemRefLoadPattern success.\n");
            return success();
        }

        llvm::outs() << "run MemRefLoadPattern failure.\n";
        return failure();
    }
};


// Transform memref::StoreOp to secret::StoreOp, convert memref type to secret type.
class MemrefStorePattern final : public OpConversionPattern<memref::StoreOp> {
public:
    using OpConversionPattern<memref::StoreOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::StoreOp op, typename memref::StoreOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        llvm::DenseMap<Value, bool> cache;
        if (!isEncrypted(op.getMemRef(), cache)) {
            return success();
        }

        Type elementTy = op.getMemRef().getType();
        if (mlir::isa<MemRefType>(elementTy)) {
            elementTy = mlir::cast<MemRefType>(elementTy).getElementType();
        }
        else {
            LLVM_DEBUG(llvm::dbgs() << "get op " << op << " type failure.\n");
            return failure();
        }

        if (auto st = mlir::dyn_cast_or_null<FloatType>(elementTy)) {
            auto memrefTy = mlir::dyn_cast<MemRefType>(op.getMemRef().getType());
            if (!memrefTy || !memrefTy.hasStaticShape()) {
                LLVM_DEBUG(llvm::dbgs() << "memrefTy:" << memrefTy << ", has static shape:" << memrefTy.hasStaticShape());
                return failure();
            }

            Value secretVal;
            if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 1) {
                int sizes = memrefTy.getShape().front();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretVectorType::get(getContext(), st, sizes), op.getMemRef());
            }
            else if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
            }

            SmallVector<Value, 8> indices(adaptor.getIndices());
            rewriter.replaceOpWithNewOp<secret::StoreOp>(op, secretVal.getType(), op.getValueToStore(), secretVal, indices);
        }
        
        LLVM_DEBUG(llvm::dbgs() << "run MemrefStorePattern success.\n");
        return success();
    }
};


// Transform affine::AffineLoadOp to secret::LoadOp, convert memref type into secret type.
class AffineLoadPattern final : public OpConversionPattern<affine::AffineLoadOp> {
public:
    using OpConversionPattern<affine::AffineLoadOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(affine::AffineLoadOp op, typename affine::AffineLoadOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        llvm::DenseMap<Value, bool> cache;
        if (!isEncrypted(op.getMemRef(), cache)) {
            return success();
        }

        auto destTy = this->getTypeConverter()->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "convert type " << op.getType() << " failure.\n");
            return failure();
        }
        
        if (auto st = mlir::dyn_cast_or_null<FloatType>(destTy))
        {
            auto memrefTy = mlir::dyn_cast<MemRefType>(op.getMemRef().getType());
            if (!memrefTy || !memrefTy.hasStaticShape()) {
                LLVM_DEBUG(llvm::dbgs() << "memrefTy:" << memrefTy << ", has static shape:" << memrefTy.hasStaticShape());
                return failure();
            }

            Value secretVal;
            if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 1) {
                int sizes = memrefTy.getShape().front();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretVectorType::get(getContext(), st, sizes), op.getMemRef());
            }

            else if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
            }

            SmallVector<Value, 8> indices(op.getMapOperands());
            auto resOperands = affine::expandAffineMap(rewriter, op.getLoc(), op.getAffineMap(), indices);
            if (!resOperands) {
                LLVM_DEBUG(llvm::dbgs() << "call affine::expandAffineMap failure.\n");
                return failure();
            }
        
            rewriter.replaceOpWithNewOp<secret::LoadOp>(op, destTy, secretVal, *resOperands);
        }

        LLVM_DEBUG(llvm::dbgs() << "run AffineLoadPattern success.\n");
        return success();
    }
};


// Transform affine::AffineStoreOp to secret::StoreOp, convert memref type to secret type.
class AffineStorePattern final : public OpConversionPattern<affine::AffineStoreOp>
{
public:
    using OpConversionPattern<affine::AffineStoreOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(affine::AffineStoreOp op, typename affine::AffineStoreOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        llvm::DenseMap<Value, bool> cache;
        if (!isEncrypted(op.getMemRef(), cache)) {
            return success();
        }
        
        Type elementTy = op.getMemRef().getType();
        if (mlir::isa<MemRefType>(elementTy)) {
            elementTy = mlir::cast<MemRefType>(elementTy).getElementType();
        }

        if (auto st = mlir::dyn_cast_or_null<FloatType>(elementTy)) {
            auto memrefTy = mlir::dyn_cast<MemRefType>(op.getMemRef().getType());
            if (!memrefTy || !memrefTy.hasStaticShape()) {
                LLVM_DEBUG(llvm::dbgs() << "memrefTy:" << memrefTy << ", has static shape:" << memrefTy.hasStaticShape());
                return failure();
            }

            Value secretVal;
            if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 1) {
                int sizes = memrefTy.getShape().front();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretVectorType::get(getContext(), st, sizes), op.getMemRef());
            }
            else if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
            }

            SmallVector<Value, 8> indices(op.getMapOperands());
            auto resOperands = affine::expandAffineMap(rewriter, op.getLoc(), op.getAffineMap(), indices);
            if (!resOperands) {
                LLVM_DEBUG(llvm::dbgs() << "call affine::expandAffineMap failure.\n");
                return failure();
            }

            rewriter.replaceOpWithNewOp<secret::StoreOp>(op, secretVal.getType(), op.getValueToStore(), secretVal, *resOperands);
        }
        
        LLVM_DEBUG(llvm::dbgs() << "run AffineStorePattern success.\n");
        return success();
    }
};


// Transform memref::AllocaOp to secret::AllocaOp
class MemrefAllocaPattern final : public OpConversionPattern<memref::AllocaOp>
{
public:
    using OpConversionPattern<memref::AllocaOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::AllocaOp op, typename memref::AllocaOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destType = this->getTypeConverter()->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "convert type " << op.getType() << " failure.\n");
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<secret::AllocaOp>(op, destType);

        LLVM_DEBUG(llvm::dbgs() << "run MemrefAllocaPattern success.\n");
        return success();
    }
};


// Transform memref::AllocOp to secret::AllocOp
class MemrefAllocPattern final : public OpConversionPattern<memref::AllocOp>
{
public:
    using OpConversionPattern<memref::AllocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::AllocOp op, typename memref::AllocOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destType = this->getTypeConverter()->convertType(op.getType());
        if (!destType) {
            LLVM_DEBUG(llvm::dbgs() << "convert type " << op.getType() << " failure.\n");
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<secret::AllocOp>(op, destType);

        LLVM_DEBUG(llvm::dbgs() << "run MemrefAllocPattern success.\n");
        return success();
    }
};


// Transform memref::DeallocOp to secret::DeallocOp
class MemrefDeallocPattern final : public OpConversionPattern<memref::DeallocOp>
{
public:
    using OpConversionPattern<memref::DeallocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::DeallocOp op, typename memref::DeallocOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        Value newOperand;
        auto o = op.getOperand();
        LLVM_DEBUG(llvm::dbgs() << o << "\n");
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
        
        
        rewriter.replaceOpWithNewOp<secret::DeallocOp>(op, newOperand);

        LLVM_DEBUG(llvm::dbgs() << "run MemrefDeallocPattern success.\n");
        return success();
    }
};


void LowerMemrefToSecretPass::getDependentDialects(mlir::DialectRegistry &registry) const  {
    registry.insert<func::FuncDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<arith::ArithDialect>();
    registry.insert<secret::SecretDialect>();
}


void LowerMemrefToSecretPass::runOnOperation() {
    auto type_converter = TypeConverter();
    type_converter.addConversion([&](Type t) {
        if (mlir::isa<MemRefType>(t)) {
            auto newTy = mlir::cast<MemRefType>(t);
            if (newTy.hasStaticShape() && newTy.getShape().size() == 1) {
                int sizes = newTy.getShape().front();
                return std::optional<Type>(SecretVectorType::get(&getContext(), newTy.getElementType(), sizes));
            }
            else if (newTy.hasStaticShape() && newTy.getShape().size() == 2) {
                int row = newTy.getShape().front();
                int col = newTy.getShape().back();
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
        if (auto ot = mlir::dyn_cast_or_null<SecretVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto oldTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(oldTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        else if (auto ot = mlir::dyn_cast_or_null<SecretMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto oldTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(oldTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }

        LLVM_DEBUG(llvm::dbgs() << "call addTargetMaterialization failure.\n");
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto ot = mlir::dyn_cast_or_null<SecretVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto oldTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(oldTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }
        else if (auto ot = mlir::dyn_cast_or_null<SecretMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto oldTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<MemRefType>(oldTy))
            {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, ot, vs));
            }
        }

        LLVM_DEBUG(llvm::dbgs() << "call addArgumentMaterialization failure.\n");
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<MemRefType>(t)) {            
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto oldTy = vs.front().getType();
            if (auto ot = mlir::dyn_cast_or_null<SecretVectorType>(oldTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
            else if (auto ot = mlir::dyn_cast_or_null<SecretMatrixType>(oldTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, destTy, vs));
            }
        }

        LLVM_DEBUG(llvm::dbgs() << "call addSourceMaterialization failure.\n");
        return std::optional<Value>(std::nullopt);
    });
    

    ConversionTarget target(getContext());
    target.addLegalDialect<func::FuncDialect, affine::AffineDialect, arith::ArithDialect, memref::MemRefDialect>();
    target.addLegalDialect<secret::SecretDialect>();
    target.addLegalOp<ModuleOp>();
    target.addIllegalOp<memref::LoadOp>();
    target.addIllegalOp<memref::StoreOp>();
    target.addIllegalOp<memref::AllocOp>();
    target.addIllegalOp<affine::AffineLoadOp>();
    target.addIllegalOp<affine::AffineStoreOp>();
    
    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<MemrefLoadPattern, MemrefStorePattern, AffineLoadPattern, AffineStorePattern,
                MemrefAllocaPattern, MemrefAllocPattern, MemrefDeallocPattern>(type_converter, patterns.getContext()); 
    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(patterns)))) {
        signalPassFailure();
    }
}


