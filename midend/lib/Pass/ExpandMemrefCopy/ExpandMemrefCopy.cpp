#include <utility>

#include "mlir/include/mlir/IR/ImplicitLocOpBuilder.h"  
#include "mlir/include/mlir/IR/PatternMatch.h"          
#include "mlir/include/mlir/IR/Visitors.h"                            
#include "mlir/include/mlir/Support/LLVM.h"              
#include "mlir/include/mlir/Support/LogicalResult.h"    
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"  
#include "mlir/include/mlir/Dialect/Affine/Analysis/LoopAnalysis.h"  
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/include/mlir/Dialect/Affine/LoopUtils.h" 
#include "mlir/include/mlir/Dialect/Affine/Utils.h"   
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"    
#include "mlir/include/mlir/Dialect/MemRef/IR/MemRef.h"  
#include "mlir/include/mlir/Dialect/SCF/IR/SCF.h"            
#include "mlir/include/mlir/Transforms/DialectConversion.h"  
#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"  
#include "Pass/ExpandMemrefCopy/ExpandMemrefCopy.h"


using namespace mlir;

SmallVector<affine::AffineForOp> expandWithAffineLoops(OpBuilder& builder, memref::CopyOp copy) {
    ImplicitLocOpBuilder b(copy.getLoc(), builder);

    // Create an affine for loop over the dimensions of the memref and
    // explicitly copy using affine loads and stores.
    MemRefType memRefType = cast<MemRefType>(copy.getSource().getType());
    SmallVector<mlir::Value, 4> indices;
    SmallVector<affine::AffineForOp> loops;

    auto zero = b.create<arith::ConstantIndexOp>(0);
    for (auto dim : memRefType.getShape()) {
        if (1 == dim) {
            // No need to create a loop for a one-dimensional index.
            indices.push_back(zero);
            continue;
        }
        auto loop = b.create<mlir::affine::AffineForOp>(0, dim);
        b.setInsertionPointToStart(loop.getBody());
        indices.push_back(loop.getInductionVar());
        loops.push_back(loop);
    }

    auto load = b.create<mlir::affine::AffineLoadOp>(copy.getSource(), indices);
    b.create<mlir::affine::AffineStoreOp>(load, copy.getTarget(), indices);
    return loops;
}


// ExpandsionMemrefCopyPattern expands a `memref.copy` with explicit affine loads and stores.
class ExpansionMemrefCopyPattern : public mlir::OpRewritePattern<mlir::memref::CopyOp> {
public:
    ExpansionMemrefCopyPattern(mlir::MLIRContext* context, bool disableAffineLoops)
        : OpRewritePattern<memref::CopyOp>(context, /*benefit=*/3),
        disableAffineLoops_(disableAffineLoops) {}

    LogicalResult matchAndRewrite(memref::CopyOp copy, PatternRewriter& rewriter) const override {
        auto nestedLoops = expandWithAffineLoops(rewriter, copy);

        if (disableAffineLoops_ && !nestedLoops.empty()) {
            nestedLoops[0].getBody(0)->walk<WalkOrder::PostOrder>([&](affine::AffineForOp forOp) {
                if (failed(loopUnrollFull(forOp))) {
                    return WalkResult::skip();
                }
                return WalkResult::advance();
                });

            if (failed(loopUnrollFull(nestedLoops[0]))) {
                return mlir::failure();
            }
        }

        rewriter.eraseOp(copy);
        return mlir::success();
    }

private:
    bool disableAffineLoops_;
};


void ExpandMemrefCopyPass::getDependentDialects(mlir::DialectRegistry &registry) const
{
    registry.insert<
        mlir::affine::AffineDialect, mlir::arith::ArithDialect, 
        mlir::memref::MemRefDialect>();
}


void ExpandMemrefCopyPass::runOnOperation() {
    GreedyRewriteConfig config;
    config.strictMode = GreedyRewriteStrictness::ExistingOps;
    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<ExpansionMemrefCopyPattern>(&getContext(), true);

    (void)applyPatternsAndFoldGreedily(getOperation(), std::move(patterns), config);
}