#ifndef PASS_BATCHING_BATCHING_H
#define PASS_BATCHING_BATCHING_H

#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

/// This pass optimizes arithmetic operations in an FHE context by converting scalar operations into SIMD-like batched
/// vector operations.
struct BatchingPass : public mlir::PassWrapper<BatchingPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "batching"; 
    }
};

#endif // PASS_BATCHING_BATCHING_H