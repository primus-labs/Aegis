#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Utils/IndexingUtils.h"
#include "mlir/Dialect/Utils/StaticValueUtils.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Sequence.h"
#include "Pass/MultiDimLoad/LowerMultiDimLoad.h"

#define DEBUG_TYPE "multi-dim-load"

using namespace mlir;
using namespace aegis;

struct LowerMultiDimLoadPattern : public OpRewritePattern<fhe::LoadOp> {
    LowerMultiDimLoadPattern(MLIRContext *context) : OpRewritePattern<fhe::LoadOp>(context, /*benefit=*/1) {}

    LogicalResult matchAndRewrite(fhe::LoadOp loadOp, PatternRewriter &rewriter) const override {
        // Check if it's multi-dimensional
        auto indices = loadOp.getIndices();
        if (indices.size() <= 1) {
            // Skip 1D load
            return failure();
        }
        if (indices.size() > 2) {
            return loadOp.emitError("Higher than two-dimensional loads are not supported.");
        }

        // Verify memref type
        auto memRefTy = mlir::dyn_cast<fhe::LWECipherMatrixType>(loadOp.getMemref().getType());
        if (!memRefTy) {
            return loadOp.emitError("Unsupported memory type ") << memRefTy;
        }

        // Get row & col form the memref type
        auto row = getConstantIntValue(indices[0]);
        auto col = getConstantIntValue(indices[1]);
        assert(row.has_value() && col.has_value());
        auto nRow = row.value();
        auto nCol = col.value();

        // Create reduced-dimension type
        mlir::Value rowVal = rewriter.create<arith::ConstantOp>(loadOp.getLoc(), rewriter.getIndexAttr(nRow));
        mlir::Value colVal = rewriter.create<arith::ConstantOp>(loadOp.getLoc(), rewriter.getIndexAttr(nCol));
        auto newMemRefType = fhe::LWECipherVectorType::get(rewriter.getContext(), memRefTy.getPlaintextType(), nCol);

        // Create vload operation
        auto vloadOp = rewriter.create<fhe::VloadOp>(
            loadOp.getLoc(), 
            newMemRefType,          // Reduced-dimension type
            loadOp.getMemref(),     // Original memref
            rowVal                  // Row index
        );

        // Create new load operation
        auto newLoadOp = rewriter.create<fhe::LoadOp>(
            loadOp.getLoc(), 
            loadOp.getType(),       // Keep original result type
            vloadOp,                // Result from vload
            colVal                  // col index
        );

        // Replace original operation
        rewriter.replaceOp(loadOp, newLoadOp);
        return success();
    }
};

void LowerMultiDimLoadPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, func::FuncDialect>();
}

void LowerMultiDimLoadPass::runOnOperation() {
    MLIRContext *ctx = &getContext();

    // Configure pattern set
    RewritePatternSet patterns(ctx);
    patterns.add<LowerMultiDimLoadPattern>(ctx);

    // Configure greedy rewrite strategy
    GreedyRewriteConfig config;
    config.useTopDownTraversal = true; // top-down traversal

    if (failed(applyPatternsAndFoldGreedily(getOperation(), std::move(patterns), config))) {
        signalPassFailure();
    }
}
