#ifndef PASS_ARITH2SECRET_LOWERARITHTOSECRET_H
#define PASS_ARITH2SECRET_LOWERARITHTOSECRET_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

struct LowerArithToSecretPass : public mlir::PassWrapper<LowerArithToSecretPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { return "arith-to-secret"; }

private:
    void collectAllMetadata(mlir::Operation *op);
};

#endif