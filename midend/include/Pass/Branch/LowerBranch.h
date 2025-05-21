#ifndef PASS_BRANCH_LOWERBRANCH_H
#define PASS_BRANCH_LOWERBRANCH_H

#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

/// This pass converts SCF conditional branches (`scf.if`) into arithmetic operations that semantically emulate a select-like behavior. 
/// By expanding the conditional execution into a data-oblivious linear combination of both branches results.
struct BranchPass : public mlir::PassWrapper<BranchPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "branch"; 
    }
};

#endif // PASS_BRANCH_LOWERBRANCH_H