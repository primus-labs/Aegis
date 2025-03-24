#include <cassert>
#include <cstdint>
#include <optional>
#include <queue>
#include <string>

#include "Pass/ExtractLoopBody/ExtractLoopBody.h"
#include "mlir/include/mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/include/mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineMemoryOpInterfaces.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineValueMap.h"
#include "mlir/include/mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/include/mlir/Dialect/Affine/Utils.h"
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/include/mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/include/mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/include/mlir/IR/IRMapping.h"
#include "mlir/include/mlir/IR/Value.h"
#include "mlir/include/mlir/IR/Visitors.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "mlir/include/mlir/Transforms/DialectConversion.h"
#include "llvm/include/llvm/ADT/TypeSwitch.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/include/llvm/Support/FormatVariadic.h"

#define DEBUG_TYPE "extract-loop-body"

using namespace llvm;
using namespace mlir;


// checkUsedForLoad checks if an operation was used to compute a load statement.
// If exclusive is true, it returns whether the op was used only to compute the
// load statement (and not in any other logic).
bool ExtractLoopBodyPass::checkUsedForLoad(Operation *op, bool exclusive) {
    std::queue<Operation *> users;
    for (auto user : op->getUsers()) {
        users.push(user);
    }

    bool usedForLoad = false;
    bool usedForStore = false;
    for (; !users.empty(); users.pop()) {
        Operation *user = users.front();

        llvm::TypeSwitch<Operation &>(*user)
            .Case<memref::LoadOp>([&](auto use) { usedForLoad = true; })
            .Case<affine::AffineLoadOp>([&](auto use) { usedForLoad = true; })
            .Case<affine::AffineStoreOp>([&](auto use) { usedForStore = true; })
            .Default([&](Operation &use) {
                for (auto user : use.getUsers()) {
                    users.push(user);
                }
            });

        if (usedForLoad && !exclusive) {
            return true;
        }

        if (usedForStore && exclusive) {
            return false;
        }
    }

    return usedForLoad;
}

// Computes the total size of the all AffineForOp.
std::optional<uint64_t> ExtractLoopBodyPass::getLoopSize(SmallVector<affine::AffineForOp> forOps) {
    uint64_t size = 1;
    for (auto forOp : forOps) {
        auto tripCount = affine::getConstantTripCount(forOp);
        if (!tripCount.has_value())
            return std::nullopt;
        size *= tripCount.value();
    }

    return size;
}

// Extracts the logic contained inside the extracts logic in the
// inner body of for loops into a function.
void ExtractLoopBodyPass::extractLoopBody(affine::AffineForOp loop, unsigned int minLoopSize, unsigned int minBodySize) {
    auto moduleLoc = loop->getParentOfType<mlir::ModuleOp>().getLoc();

    SmallVector<affine::AffineForOp> nestedLoops;
    affine::getPerfectlyNestedLoops(nestedLoops, loop);

    // Check total size of loop.
    auto loopSize = getLoopSize(nestedLoops);
    if (!loopSize.has_value() || loopSize.value() < minLoopSize) {
        return;
    }

    // Walk the inner loop body, collect func args, funcs logic, and result.
    SmallVector<Type, 4> inputTypes;
    SmallVector<Value, 4> inputs;
    SmallVector<Value> results;
    SmallVector<Operation *, 10> opsToCopy;
    SmallPtrSet<Operation *, 4> constantOps;

    mlir::Region &innerLoopBody = nestedLoops[nestedLoops.size() - 1].getRegion();
    innerLoopBody.walk<WalkOrder::PreOrder>([&](Operation *op) {
        return llvm::TypeSwitch<Operation &, WalkResult>(*op)
            .Case<affine::AffineLoadOp, memref::LoadOp>([&](auto op) {
                inputTypes.push_back(op.getResult().getType());
                inputs.push_back(op.getResult());
                return WalkResult::advance();
            })
            .Case<affine::AffineStoreOp>([&](auto op) {
                results.push_back(op.getValue());
                return WalkResult::advance();
            })
            .Case<affine::AffineYieldOp>([&](auto op) { return WalkResult::skip(); })
            .Default([&](Operation &op) {
                // Don't copy operations only used to compute load statements.
                if (checkUsedForLoad(&op, /*exclusive=*/true))
                    return WalkResult::advance();
                opsToCopy.push_back(&op);
                for (auto operand : op.getOperands()) {
                    // if the operand is block argument, getDefiningOp return null, then continue;
                    if (!operand.getDefiningOp()) {
                        continue;
                    }
                    assert(operand.getDefiningOp() != nullptr);
                    if (isa<arith::ConstantOp>(operand.getDefiningOp())) {
                        constantOps.insert(operand.getDefiningOp());
                    }
                }
                return WalkResult::advance();
            });
    });

    // Nothing to do if there are no operations to copy into a function.
    if (opsToCopy.empty()) {
        return;
    }

    if (inputTypes.empty() || results.size() != 1) {
        loop.emitWarning("extract-loop-body only supports loops with a loads and a single store "
                         "operation");
        return;
    }

    auto result = results[0];
    auto finalOp = opsToCopy[opsToCopy.size() - 1];
    if (finalOp->getNumResults() != 1 && finalOp->getResult(0) != result) {
        loop.emitWarning("expected loop body to terminate with a single final store operation");
        return;
    }

    if (opsToCopy.size() < minBodySize) {
        return;
    }

    // Create the new function.
    OpBuilder builder(loop->getParentOfType<func::FuncOp>());
    auto type = builder.getFunctionType(inputTypes, result.getType());
    std::string funcName = llvm::formatv("for_{0}", mlir::hash_value(result));
    auto funcOp = builder.create<func::FuncOp>(moduleLoc, funcName, type);

    // Populate function body by cloning the ops in the inner body and mapping
    // the func args and func outputs.
    Block *block = funcOp.addEntryBlock();
    builder.setInsertionPointToStart(block);

    // Map the input values to the block arguments.
    IRMapping mp;
    for (size_t index = 0; index < inputs.size(); ++index) {
        mp.map(inputs[index], block->getArgument(index));
    }

    // Build the function body.
    Operation *clonedOp;
    for (auto op : constantOps) {
        clonedOp = builder.clone(*op, mp);
    }
    for (auto op : opsToCopy) {
        clonedOp = builder.clone(*op, mp);
    }

    // Add a return statement for the final cloned operation's result.
    builder.create<func::ReturnOp>(funcOp.getLoc(), clonedOp->getResult(0));

    // Call the function.
    builder.setInsertionPointAfter(result.getDefiningOp());
    auto callOp = builder.create<func::CallOp>(result.getLoc(), funcOp, inputs);
    result.getDefiningOp()->replaceAllUsesWith(callOp);

    // Erase previous ops, except for the load statements and its dependents.
    for (SmallVectorImpl<Operation *>::reverse_iterator rit = opsToCopy.rbegin(); rit != opsToCopy.rend(); ++rit) {
        auto op = *rit;
        // If the operation was also used for a load statement, don't erase.
        if (checkUsedForLoad(op, /*exclusive=*/false)) 
            continue;
        op->erase();
    }
}

void ExtractLoopBodyPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<affine::AffineDialect, arith::ArithDialect, memref::MemRefDialect>();
}

void ExtractLoopBodyPass::runOnOperation() {
    auto module = getOperation();

    module.walk([&](affine::AffineForOp op) {
        if (!isa<func::FuncOp>(op.getOperation()->getParentOp())) {
            return;
        }
        extractLoopBody(op, /*minLoopSize*/8, /*minBodySize*/3);
    });
}