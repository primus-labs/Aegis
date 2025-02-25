#include <utility>

#include "llvm/include/llvm/ADT/TypeSwitch.h"            
#include "llvm/include/llvm/Support/Debug.h" 
#include "mlir/include/mlir/Support/LLVM.h"              
#include "mlir/include/mlir/Support/LogicalResult.h"               
#include "mlir/include/mlir/Dialect/Affine/Utils.h"      
#include "mlir/include/mlir/Dialect/Tensor/IR/Tensor.h" 
#include "mlir/include/mlir/IR/MLIRContext.h"            
#include "mlir/include/mlir/IR/PatternMatch.h"    
 #include "mlir/include/mlir/IR/Dominance.h"         
#include "mlir/include/mlir/IR/Value.h"                 
#include "mlir/include/mlir/IR/ValueRange.h"               
#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"  
#include "Pass/ForwardInsertToExtract/ForwardInsertToExtract.h"

#define DEBUG_TYPE "forward-insert-to-extract"

using namespace mlir;


struct ForwardSingleInsertToExtract: public OpRewritePattern<tensor::ExtractOp> {
    ForwardSingleInsertToExtract(mlir::MLIRContext *context, DominanceInfo &dom)
        : OpRewritePattern<tensor::ExtractOp>(context, 3), domInfo(dom) {}

public:
    LogicalResult matchAndRewrite(tensor::ExtractOp extractOp, PatternRewriter &rewriter) const override {
        LLVM_DEBUG(llvm::dbgs() << "Considering extractOp for replacement: " << extractOp << "\n");

        auto *def = extractOp.getTensor().getDefiningOp();
        if (def != nullptr) {
            LLVM_DEBUG(llvm::dbgs() << "DefiningOp of the one considered: "
                    << *extractOp.getTensor().getDefiningOp<tensor::InsertOp>() << "\n");

            LLVM_DEBUG(llvm::dbgs() << "Considering def for forwarding: " << *def << "\n");
            if (isForwardableOp(def, extractOp)) {
                auto result = getInsertedValue(def);
                LLVM_DEBUG(llvm::dbgs() << "def is forwardable: " << *def << "\n");
                if (failed(result)) {
                    return failure();
                }
                auto value = result.value();
                rewriter.replaceAllUsesWith(extractOp, value);
                return success();
            }
            LLVM_DEBUG(llvm::dbgs() << "def is not forwardable: " << *def << "\n");
        } 
        else {
            LLVM_DEBUG(llvm::dbgs() << "def is nullptr " << "\n");
        }

        return failure();
    }

private:
    bool isForwardableOp(Operation *maybeInsert, tensor::ExtractOp &extractOp) const {
        if (!domInfo.properlyDominates(maybeInsert, extractOp.getOperation())) {
            LLVM_DEBUG(llvm::dbgs() << "insert op does not dominate extract op.\n");
            return false;
        }

        if (extractOp->getBlock() != maybeInsert->getBlock()) {
            LLVM_DEBUG(llvm::dbgs() << "insert and extract op are not in the same block.\n");
            return false;
        }

        return llvm::TypeSwitch<Operation &, bool>(*maybeInsert).Case<tensor::InsertOp>([&](auto insertOp) {
                ValueRange insertIndices = insertOp.getIndices();
                ValueRange extractIndices = extractOp.getIndices();
                if (insertIndices != extractIndices) {
                    LLVM_DEBUG(llvm::dbgs() << "insert and extract op do not have matching indices.\n");
                    return false;
                }

                // Naively scan through the operations between the two ops and check if
                // anything prevents forwarding.
                for (auto currentNode = insertOp->getNextNode();
                    currentNode != extractOp.getOperation();
                    currentNode = currentNode->getNextNode()) {
                    if (currentNode->getNumRegions() > 0) {
                        LLVM_DEBUG(llvm::dbgs() << "an op with control flow is between the insert and extract op.\n");
                        return false;
                    }

                    if (auto op = dyn_cast<tensor::InsertOp>(currentNode)) {
                        if (op.getDest() == insertOp.getDest() &&
                            op.getIndices() == insertIndices) {
                            LLVM_DEBUG(llvm::dbgs() << "an intermediate op inserts to the same index.\n");
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

    FailureOr<Value> getInsertedValue(Operation *insertOp) const {
        return llvm::TypeSwitch<Operation &, FailureOr<Value>>(*insertOp)
            .Case<tensor::InsertOp>([&](auto insertOp) { return insertOp.getScalar(); })
            .Default([&](Operation &) { return failure(); });
    }

private:
    DominanceInfo &domInfo;
};


void ForwardInsertToExtractPass::runOnOperation() {
    MLIRContext *context = &getContext();
    RewritePatternSet patterns(context);
    DominanceInfo dom(getOperation());
    patterns.add<ForwardSingleInsertToExtract>(context, dom);
    (void)applyPatternsAndFoldGreedily(getOperation(), std::move(patterns));  
}