#include <utility>

#include "llvm/include/llvm/ADT/TypeSwitch.h"  
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h"              
#include "mlir/include/mlir/Support/LogicalResult.h"     
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h" 
#include "mlir/include/mlir/Dialect/Affine/Utils.h"      
#include "mlir/include/mlir/Dialect/MemRef/IR/MemRef.h" 
#include "mlir/include/mlir/IR/MLIRContext.h"            
#include "mlir/include/mlir/IR/PatternMatch.h"           
#include "mlir/include/mlir/IR/Value.h"                  
#include "mlir/include/mlir/IR/ValueRange.h"            
#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/include/mlir/IR/Dominance.h" 
#include "Pass/ForwardStoreToLoad/ForwardStoreToLoad.h"

#define DEBUG_TYPE "forward-store-to-load"

using namespace mlir;


/// Transforms an 'affine.store' operation by applying its affine map to the operands and 
/// replacing it with a 'memref.store' operation.
class AffineStorePattern : public OpRewritePattern<affine::AffineStoreOp> {
public:
    using OpRewritePattern<affine::AffineStoreOp>::OpRewritePattern;

    LogicalResult matchAndRewrite(affine::AffineStoreOp op, PatternRewriter &rewriter) const override {
        // Expand affine map from 'affineStoreOp'.
        SmallVector<Value, 8> indices(op.getMapOperands());
        auto maybeExpandedMap = affine::expandAffineMap(rewriter, op.getLoc(), op.getAffineMap(), indices);
        if (!maybeExpandedMap)  {
            return failure();
        }

        // Build memref.store valueToStore, memref[expandedMap.results].
        rewriter.replaceOpWithNewOp<memref::StoreOp>(op, op.getValueToStore(), op.getMemRef(), *maybeExpandedMap);
        return success();
}
};

/// Transforms an 'affine.load' operation by applying its affine map to the operands and 
/// replacing it with a 'memref.load' operation.
class AffineLoadPattern : public OpRewritePattern<affine::AffineLoadOp> {
public:
    using OpRewritePattern<affine::AffineLoadOp>::OpRewritePattern;

    LogicalResult matchAndRewrite(affine::AffineLoadOp op, PatternRewriter &rewriter) const override {
        // Expand affine map from 'affineLoadOp'.
        SmallVector<Value, 8> indices(op.getMapOperands());
        auto resOps = affine::expandAffineMap(rewriter, op.getLoc(), op.getAffineMap(), indices);
        if (!resOps)  {
            return failure();
        }

        // Build vector.load memref[expandedMap.results].
        rewriter.replaceOpWithNewOp<memref::LoadOp>(op, op.getMemRef(), *resOps);
        return success();
    }
};


// Find a memref load and try to forward the most recent store op.
struct ForwardSingleStoreToLoadPattern : public OpRewritePattern<memref::LoadOp> {
    ForwardSingleStoreToLoadPattern(mlir::MLIRContext *context, DominanceInfo &dom)
        : OpRewritePattern<memref::LoadOp>(context, /*benefit=*/3), domInfo(dom) {}

public:
    LogicalResult matchAndRewrite(memref::LoadOp loadOp, PatternRewriter &rewriter) const override {
        LLVM_DEBUG(llvm::dbgs() << "Considering loadOp for replacement: " << loadOp << "\n");
        for (Operation *use : loadOp.getMemRef().getUsers()) {
            LLVM_DEBUG(llvm::dbgs() << "Considering memref use for forwarding: " << *use << "\n");
            if (isForwardableOp(use, loadOp)) {
                auto result = getStoredValue(use);
                LLVM_DEBUG(llvm::dbgs() << "Use is forwardable: " << *use << "\n");
                if (failed(result)) {
                    return failure();
                }
                
                auto value = result.value();
                rewriter.replaceAllUsesWith(loadOp, value);
                return success();
            }
            LLVM_DEBUG(llvm::dbgs() << "Use is not forwardable: " << *use << "\n");
        }
        return failure();
    }

private:
    // Updates an internal cache with results of this query so they can be used recursively.
    bool isForwardableOp(Operation *maybeStore, memref::LoadOp &loadOp) const {
        if (!domInfo.properlyDominates(maybeStore, loadOp.getOperation())) {
            LLVM_DEBUG(llvm::dbgs() << "store op does not dominate load op.\n");
            return false;
        }

        // Probably want to relax this at some point in the future.
        if (loadOp->getBlock() != maybeStore->getBlock()) {
            LLVM_DEBUG(llvm::dbgs() << "loadOp and store op are not in the same block.\n");
            return false;
        }

        return llvm::TypeSwitch<Operation &, bool>(*maybeStore).Case<memref::StoreOp>([&](auto storeOp) {
            ValueRange storeIndices = storeOp.getIndices();
            ValueRange loadIndices = loadOp.getIndices();
            if (storeIndices != loadIndices) {
                LLVM_DEBUG(llvm::dbgs() << "loadOp and store op do not have matching indices.\n");
                return false;
            }

            // Scan the operations between the two ops to check for anything prevents forwarding.
            for (auto currentNode = storeOp->getNextNode();
                    currentNode != loadOp.getOperation();
                    currentNode = currentNode->getNextNode()) {
                if (currentNode->getNumRegions() > 0) {
                    // Op can have control flow
                    LLVM_DEBUG(llvm::dbgs() << "an op with control flow is between the store and load op.\n");
                    return false;
                }

                if (auto op = dyn_cast<memref::StoreOp>(currentNode)) {
                    if (op.getMemRef() == storeOp.getMemRef() && op.getIndices() == storeIndices) {
                        LLVM_DEBUG(llvm::dbgs() << "an intermediate op stores to the same index.\n");
                        return false;
                    }
                }
            }

            return true;

            }).Default([&](Operation &) {
                LLVM_DEBUG(llvm::dbgs() << "Unsupported op type, cannot check for forwardability.\n");
                return false;
            });
    }

    FailureOr<Value> getStoredValue(Operation *storeOp) const {
        return llvm::TypeSwitch<Operation &, FailureOr<Value>>(*storeOp)
                .Case<memref::StoreOp>([&](auto storeOp) { return storeOp.getValueToStore(); })
                .Default([&](Operation &) { return failure(); });
    }

private:
    DominanceInfo &domInfo;
};


// Eliminate unused store op
struct RemoveUnusedStorePattern : public OpRewritePattern<memref::StoreOp> {
    RemoveUnusedStorePattern(mlir::MLIRContext *context, DominanceInfo &dom)
        : OpRewritePattern<memref::StoreOp>(context, /*benefit=*/3), domInfo(dom) {}

public:
    LogicalResult matchAndRewrite(memref::StoreOp storeOp, PatternRewriter &rewriter) const override {
        LLVM_DEBUG(llvm::dbgs() << "Considering storeOp for removal: " << storeOp << "\n");

        for (Operation *use : storeOp.getMemRef().getUsers()) {
            if (isPostDominated(use, storeOp)) {
                LLVM_DEBUG(llvm::dbgs() << "Store is usurped by: " << *use << "\n");
                rewriter.eraseOp(storeOp);
                return success();
            }

            LLVM_DEBUG(llvm::dbgs() << "Use is not removable: " << *use << "\n");
        }

        return failure();
    }

private:
    bool isPostDominated(Operation *maybeStoreOp, memref::StoreOp &storeOp) const {
        if (!domInfo.properlyDominates(storeOp.getOperation(), maybeStoreOp)) {
            LLVM_DEBUG(llvm::dbgs() << "store op is not properly dominated by potential store.\n");
            return false;
        }   

        // Probably want to relax this at some point in the future.
        if (storeOp->getBlock() != maybeStoreOp->getBlock()) {
            LLVM_DEBUG(llvm::dbgs() << "store ops are not in the same block\n");
            return false;
        }

        return llvm::TypeSwitch<Operation &, bool>(*maybeStoreOp).Case<memref::StoreOp>([&](auto potentialStore) {
            ValueRange storeIndices = storeOp.getIndices();
            ValueRange potentialStoreIndices = potentialStore.getIndices();
            if (storeIndices != potentialStoreIndices) {
                LLVM_DEBUG(llvm::dbgs() << "store ops do not have matching indices.\n");
                return false;
            }

            // Scan through the operations between the two ops and check if a read prevents store removal.
            for (auto currentNode = storeOp->getNextNode();
                currentNode != potentialStore.getOperation();
                currentNode = currentNode->getNextNode()) {
                if (currentNode->getNumRegions() > 0) {
                    // Op can have control flow
                    LLVM_DEBUG(llvm::dbgs() << "an op with control flow is between the store ops.\n");
                    return false;
                }
                if (auto op = dyn_cast<affine::AffineLoadOp>(currentNode)) {
                    // If we encounter an affine load op then fail conservatively. This
                    // pass should have already run AffineLoadLowering to convert all
                    // possible affine loads to memref loads.
                    LLVM_DEBUG(llvm::dbgs() << "an intermediate load op was found.\n");
                    return false;
                }
                if (auto op = dyn_cast<memref::LoadOp>(currentNode)) {
                    if (op.getMemRef() == storeOp.getMemRef() && op.getIndices() == storeIndices) {
                        LLVM_DEBUG(llvm::dbgs() << "an intermediate op loads at the same index.\n");
                        return false;
                    }
                }
            }

            return true;

        }).Default([&](Operation &) {
            LLVM_DEBUG(llvm::dbgs() << "Unsupported op type, cannot check for forwardability.\n");
            return false;
        });
    }

private:
    DominanceInfo &domInfo;
};


void ForwardStoreToLoadPass::getDependentDialects(mlir::DialectRegistry &registry) const
{
    registry.insert<mlir::affine::AffineDialect, mlir::memref::MemRefDialect>();
}


void ForwardStoreToLoadPass::runOnOperation() {
    MLIRContext *context = &getContext();
    RewritePatternSet patterns(context);
    DominanceInfo dom(getOperation());
    patterns.add<AffineLoadPattern, AffineStorePattern>(context);
    patterns.add<ForwardSingleStoreToLoadPattern, RemoveUnusedStorePattern>(context, dom);
    (void)applyPatternsAndFoldGreedily(getOperation(), std::move(patterns));
}