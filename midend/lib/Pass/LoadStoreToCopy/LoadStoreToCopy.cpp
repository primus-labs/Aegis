#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Sequence.h"
#include "Pass/LoadStoreToCopy/LoadStoreToCopy.h"

#define DEBUG_TYPE "loadstore-to-copy"

using namespace mlir;
using namespace aegis;


struct LoadStoreToCopyPattern : public OpRewritePattern<fhe::StoreOp> {
    LoadStoreToCopyPattern(MLIRContext *context) : OpRewritePattern<fhe::StoreOp>(context, /*benefit=*/1) {}

    LogicalResult matchAndRewrite(fhe::StoreOp storeOp, PatternRewriter &rewriter) const override {
        // Verify store value comes from load operation
        auto loadOp = storeOp.getValueToStore().getDefiningOp<fhe::LoadOp>();
        if (!loadOp) {
            LLVM_DEBUG(llvm::dbgs() << "Store value not from LoadOp\n");
            return failure();
        }

        // Check index is constant
        if (storeOp.getIndices().size() > 1) {
            LLVM_DEBUG(llvm::dbgs() << "StoreOp Indices size > 1\n");
            return failure();
        }
        auto constIndex = storeOp.getIndices()[0].getDefiningOp<arith::ConstantIndexOp>();
        if (!constIndex) {
            LLVM_DEBUG(llvm::dbgs() << "Non-constant index\n");
            return failure();
        }

        // Validate vector types
        Value sourceVec = loadOp.getMemref();
        Value destVec = storeOp.getMemref();
        auto sourceType = mlir::dyn_cast<fhe::LWECipherVectorType>(sourceVec.getType());
        auto destType = mlir::dyn_cast<fhe::LWECipherVectorType>(destVec.getType());
        if (!sourceType || !destType || sourceType != destType) {
            LLVM_DEBUG({
                llvm::dbgs() << "Type mismatch\n";
                if (!sourceType) llvm::dbgs() << "  Source is not LWECipherVecType\n";
                if (!destType) llvm::dbgs() << "  Dest is not LWECipherVecType\n";
                if (sourceType != destType) 
                    llvm::dbgs() << "  Type mismatch: " << sourceType << " vs " << destType << "\n";
            });
            return failure();
        }

        // Check source type size equal the dest type size
        const int64_t srcVecSize  = sourceType.getSize();
        const int64_t destVecSize = destType.getSize();
        if (srcVecSize != destVecSize) {
            LLVM_DEBUG(llvm::dbgs() << "source vector size not equal dest vector size\n");
            return failure();
        }

        // Collect all related operations
        DenseMap<int64_t, std::pair<fhe::StoreOp, fhe::LoadOp>> opsMap;
        for (Operation &op : *storeOp->getBlock()) {
            if (auto candidateStore = dyn_cast<fhe::StoreOp>(&op)) {
                // Must use same destination vector
                if (candidateStore.getMemref() != destVec) {
                    continue;
                }

                // Must load from same source vector
                auto candidateLoad = candidateStore.getValueToStore().getDefiningOp<fhe::LoadOp>();
                if (!candidateLoad || candidateLoad.getMemref() != sourceVec) {
                    continue;
                }

                // Must use constant index in valid range
                if (candidateStore.getIndices().size() > 1) {
                    LLVM_DEBUG(llvm::dbgs() << "StoreOp Indices size > 1\n");
                    continue;
                }
                auto idx = candidateStore.getIndices()[0].getDefiningOp<arith::ConstantIndexOp>();
                if (!idx || idx.value() < 0 || idx.value() >= srcVecSize) {
                    continue;
                }

                opsMap.try_emplace(idx.value(), candidateStore, candidateLoad);
            }
        }

        // Step 5: Verify complete index coverage
        if (opsMap.size() != srcVecSize) {
            LLVM_DEBUG(llvm::dbgs() << "Index coverage incomplete ("
                                    << opsMap.size() << "/" << srcVecSize << ")\n");
            return failure();
        }
        for (int64_t i = 0; i < srcVecSize; ++i) {
            if (!opsMap.count(i)) {
                LLVM_DEBUG(llvm::dbgs() << "Missing index " << i << "\n");
                return failure();
            }
        }

        // Enhanced safety checks
        Operation* lastStore = opsMap[srcVecSize-1].first;
        Operation* copyInsertPoint = lastStore->getNextNode();
        bool hasDangerousOp = false;

        // Get all collected operations for quick lookup
        DenseSet<Operation*> collectedOperations;
        for (auto &entry : opsMap) {
            collectedOperations.insert(entry.second.first);  // Store ops
            collectedOperations.insert(entry.second.second); // Load ops
        }

        for (Operation& op : *storeOp->getBlock()) {
            // Stop at insertion point
            if (&op == copyInsertPoint) {
                break;
            }

            // Skip collected operations
            if (collectedOperations.contains(&op)) {
                continue;
            }

            // Any operand uses source/dest vector(Memref)
            for (Value operand : op.getOperands()) {
                if (operand == sourceVec || operand == destVec) {
                    hasDangerousOp = true;
                    break;
                }
            }
            if (hasDangerousOp) {
                break;
            }

            // Any operation using the vectors
            if (auto memOp = dyn_cast<fhe::LoadOp>(&op)) {
                if (memOp.getMemref() == sourceVec || memOp.getMemref() == destVec) {
                    hasDangerousOp = true;
                    break;
                }
            }
            if (auto memOp = dyn_cast<fhe::StoreOp>(&op)) {
                if (memOp.getMemref() == destVec) {
                    hasDangerousOp = true;
                    break;
                }
            }
        }

        if (hasDangerousOp) {
            return failure(); 
        }

        // Create vector copy and clean up
        LLVM_DEBUG(llvm::dbgs() << "Optimizing " << srcVecSize
                                << "-element copy at " << storeOp->getLoc() << "\n");
        rewriter.setInsertionPointAfter(lastStore);
        rewriter.create<fhe::CopyOp>(storeOp.getLoc(), sourceVec, destVec);
        
        // Erase in reverse order to maintain dominance
        SmallVector<Operation*> toErase;
        for (auto &entry : opsMap) {
            toErase.push_back(entry.second.first);  // Store ops
            toErase.push_back(entry.second.second); // Load ops
        }
        for (auto it = toErase.rbegin(); it != toErase.rend(); ++it) {
            rewriter.eraseOp(*it);
        }

        return success();
    }
};

/// Convert scalar fhe::load/fhe::store sequences to fhe::copy operations
void LoadStoreToCopyPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, func::FuncDialect>();
}


void LoadStoreToCopyPass::runOnOperation() {
    RewritePatternSet patterns(&getContext());
    patterns.add<LoadStoreToCopyPattern>(&getContext());
    
    if (failed(applyPatternsAndFoldGreedily(getOperation(), std::move(patterns)))) {
      signalPassFailure();
    }
}