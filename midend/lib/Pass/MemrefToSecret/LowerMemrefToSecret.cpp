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
                assert(secretVal);
            }
            else if (memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                        SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
                assert(secretVal);
            } 
            else {
                LLVM_DEBUG(llvm::dbgs() << "unsupport shape for " << memrefTy.getShape().size() << ".\n");
                return failure();
            }

            SmallVector<Value, 8> indices(adaptor.getIndices());
            auto secretLoadTy = SecretType::get(getContext(), destTy);
            auto secretLoadOp = rewriter.replaceOpWithNewOp<secret::LoadOp>(op, secretLoadTy, secretVal, indices);

            // Handle all associated secret.cast operations.
            for (auto user : llvm::make_early_inc_range(op->getUsers())) {
                if (auto castOp = dyn_cast<secret::CastOp>(user)) {
                    if (castOp.getType() == secretLoadTy) {
                        rewriter.replaceOp(castOp, secretLoadOp.getResult());
                    }
                }
            }

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
        if (!isEncrypted(op.getMemRef(), cache) && !isEncrypted(op.getValueToStore(), cache)) {
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

            Value secretArrVal;
            if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 1) {
                int sizes = memrefTy.getShape().front();
                secretArrVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretVectorType::get(getContext(), st, sizes), op.getMemRef());
                assert(secretArrVal);
            }
            else if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretArrVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
                assert(secretArrVal);
            }

            SmallVector<Value, 8> indices(adaptor.getIndices());
            auto memValToStore = op.getValueToStore();
            auto secretValToStroe = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(), 
                                                            SecretType::get(getContext(), memValToStore.getType()), memValToStore);
            assert(secretValToStroe);
            rewriter.replaceOpWithNewOp<secret::StoreOp>(op, secretValToStroe, secretArrVal, indices);
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
                assert(secretVal);
            }

            else if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
                assert(secretVal);
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
                assert(secretVal);
            }
            else if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                secretVal = typeConverter->materializeTargetConversion(rewriter, op.getMemRef().getLoc(),
                                            SecretMatrixType::get(getContext(), st, row, col), op.getMemRef());
                assert(secretVal);
            }

            SmallVector<Value, 8> indices(op.getMapOperands());
            auto resOperands = affine::expandAffineMap(rewriter, op.getLoc(), op.getAffineMap(), indices);
            if (!resOperands) {
                LLVM_DEBUG(llvm::dbgs() << "call affine::expandAffineMap failure.\n");
                return failure();
            }

            rewriter.replaceOpWithNewOp<secret::StoreOp>(op, op.getValueToStore(), secretVal, *resOperands);
        }
        
        LLVM_DEBUG(llvm::dbgs() << "run AffineStorePattern success.\n");
        return success();
    }
};


// Transform memref::CopyOp to secret::CopyOp, convert memref type to secret type.
class MemrefCopyPattern final : public OpConversionPattern<memref::CopyOp> {
public:
    using OpConversionPattern<memref::CopyOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::CopyOp op, typename memref::CopyOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        llvm::DenseMap<Value, bool> cache;
        if (!isEncrypted(op.getSource(), cache) && !isEncrypted(op.getTarget(), cache)) {
            return success();
        }

        auto fnGetVal = [op, &rewriter, this](Value SrcOrDestVal) -> std::optional<Value> {
            Type elementTy = SrcOrDestVal.getType();
            if (mlir::isa<MemRefType>(elementTy)) {
                elementTy = mlir::cast<MemRefType>(elementTy).getElementType();
            }
            else {
                LLVM_DEBUG(llvm::dbgs() << "get memref::copyop op " << op << " type failure.\n");
                return std::optional<Value>(std::nullopt);
            }

            auto memrefTy = mlir::dyn_cast<MemRefType>(SrcOrDestVal.getType());
            if (!memrefTy || !memrefTy.hasStaticShape()) {
                LLVM_DEBUG(llvm::dbgs() << "memrefTy:" << memrefTy << ", has static shape:" << memrefTy.hasStaticShape());
                return std::optional<Value>(std::nullopt);
            }

            Value newSrcOrDestVal;
            if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 1) {
                int sizes = memrefTy.getShape().front();
                newSrcOrDestVal = typeConverter->materializeTargetConversion(rewriter, SrcOrDestVal.getLoc(),
                                            SecretVectorType::get(getContext(), elementTy, sizes), SrcOrDestVal);
                assert(newSrcOrDestVal);
            }
            else if (memrefTy.hasStaticShape() && memrefTy.getShape().size() == 2) {
                int row = memrefTy.getShape().front();
                int col = memrefTy.getShape().back();
                newSrcOrDestVal = typeConverter->materializeTargetConversion(rewriter, SrcOrDestVal.getLoc(),
                                            SecretMatrixType::get(getContext(), elementTy, row, col), SrcOrDestVal);
                assert(newSrcOrDestVal);
            }
            else {
                llvm::outs() << "Unsupport rank:" << memrefTy.getShape().size() << ".\n";
                return std::optional<Value>(std::nullopt);
            }

            return std::optional<Value>(newSrcOrDestVal);
        };

        auto newSrcVal = fnGetVal(op.getSource());
        auto newDestVal = fnGetVal(op.getTarget());
        rewriter.replaceOpWithNewOp<secret::CopyOp>(op, newSrcVal.value(), newDestVal.value());
        
        LLVM_DEBUG(llvm::dbgs() << "run MemrefCopyPattern success.\n");
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

        IntegerAttr alignmentAttr;
        auto alignment = op.getAlignment();
        if (alignment.has_value())
            alignmentAttr = rewriter.getI64IntegerAttr(alignment.value());
    
        rewriter.replaceOpWithNewOp<secret::AllocOp>(op, destType, alignmentAttr);

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
            else if (newTy.hasStaticShape() && newTy.getShape().size() > 2) {
                llvm::errs() << "Currently, static shapes larger than 2 are not supported.";
                return std::optional<Type>(t);
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
        if (mlir::isa<SecretType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<mlir::FloatType, mlir::IntegerType, mlir::IndexType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, t, vs));
            }
        }
        else if (mlir::isa<SecretVectorType, SecretMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<MemRefType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "[MemrefToSecretPass] Materialization(addTargetMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<SecretType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<mlir::FloatType, mlir::IntegerType, mlir::IndexType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, t, vs));
            }
        }
        else if (mlir::isa<SecretVectorType, SecretMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<MemRefType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "[MemrefToSecretPass] Materialization(addArgumentMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<MemRefType>(t)) {            
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<SecretVectorType, SecretMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<secret::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "[MemrefToSecretPass] Materialization(addSourceMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });
    

    ConversionTarget target(getContext());
    target.addLegalDialect<func::FuncDialect, affine::AffineDialect, arith::ArithDialect, memref::MemRefDialect>();
    target.addLegalDialect<secret::SecretDialect>();
    target.addLegalOp<ModuleOp>();
    target.addIllegalOp<memref::LoadOp>();
    target.addIllegalOp<memref::StoreOp>();
    target.addIllegalOp<memref::AllocOp>();
    target.addIllegalOp<memref::CopyOp>();
    target.addIllegalOp<affine::AffineLoadOp>();
    target.addIllegalOp<affine::AffineStoreOp>();
    
    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<MemrefLoadPattern, MemrefStorePattern, AffineLoadPattern, AffineStorePattern, MemrefCopyPattern,
                MemrefAllocaPattern, MemrefAllocPattern, MemrefDeallocPattern>(type_converter, patterns.getContext()); 
    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(patterns)))) {
        signalPassFailure();
    }
}


