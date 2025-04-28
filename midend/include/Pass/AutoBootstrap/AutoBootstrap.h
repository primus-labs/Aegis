#ifndef PASS_AUTOBOOTSTRAP_AUTOBOOTSTRAP_H
#define PASS_AUTOBOOTSTRAP_AUTOBOOTSTRAP_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

struct AutoBootstrapPass : public mlir::PassWrapper<AutoBootstrapPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "auto-bootstrap"; 
    }

};

#endif  // PASS_AUTOBOOTSTRAP_AUTOBOOTSTRAP_H