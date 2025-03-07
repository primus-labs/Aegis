#ifndef PASS_MEMREFTOSECRET_LOWERMEMREFTOSECRET_H
#define PASS_MEMREFTOSECRET_LOWERMEMREFTOSECRET_H


#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"


struct LowerMemrefToSecretPass : public mlir::PassWrapper<LowerMemrefToSecretPass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "memref-to-secret";
    }
};

#endif // PASS_MEMREFTOSECRET_LOWERMEMREFTOSECRET_H