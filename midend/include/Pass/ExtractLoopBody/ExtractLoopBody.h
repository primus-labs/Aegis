#ifndef PASS_EXTRACTLOOPBODY_EXTRACTLOOPBODY_H
#define PASS_EXTRACTLOOPBODY_EXTRACTLOOPBODY_H

#include "mlir/Pass/Pass.h"
#include "llvm/include/llvm/ADT/SmallVector.h"
#include "mlir/include/mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/include/mlir/IR/DialectRegistry.h"


/// ExtractLoopBodyPass extracts loop body logic into functions in the case where
/// a loop is loading values from memrefs, computing some function, and then
/// storing the result in an output memref. The function inputs become the loaded
/// values, and the function output is the value to store.
struct ExtractLoopBodyPass : public mlir::PassWrapper<ExtractLoopBodyPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { return "extract-loop-body"; }

private:
    void extractLoopBody(mlir::affine::AffineForOp loop, unsigned int minLoopSize, unsigned int minBodySize);
    std::optional<uint64_t> getLoopSize(llvm::SmallVector<mlir::affine::AffineForOp> forOps);
    bool checkUsedForLoad(mlir::Operation *op, bool exclusive);
};

#endif // PASS_EXTRACTLOOPBODY_EXTRACTLOOPBODY_H