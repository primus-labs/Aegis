#ifndef PASS_FOLDARITHCHAIN_FOLDARITHCHAIN_H
#define PASS_FOLDARITHCHAIN_FOLDARITHCHAIN_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

struct FoldArithChainPass : public mlir::PassWrapper<FoldArithChainPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;
    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "fold-arith-chain"; 
    }
};

#endif // PASS_FOLDARITHCHAIN_FOLDARITHCHAIN_H
