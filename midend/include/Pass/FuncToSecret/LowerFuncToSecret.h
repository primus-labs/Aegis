#ifndef PASS_FUNCTOSECRET_LOWERFUNCTOSECRET_H
#define PASS_FUNCTOSECRET_LOWERFUNCTOSECRET_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

struct LowerFuncToSecretPass : public mlir::PassWrapper<LowerFuncToSecretPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { return "func-to-secret"; }
};

#endif // PASS_FUNCTOSECRET_LOWERFUNCTOSECRET_H