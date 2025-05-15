#ifndef PASS_LOADSTORETOCOPY_LOADSTORETOCOPY_H
#define PASS_LOADSTORETOCOPY_LOADSTORETOCOPY_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

struct LoadStoreToCopyPass : public mlir::PassWrapper<LoadStoreToCopyPass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "loadstore-to-copy"; 
    }
};

#endif // PASS_LOADSTORETOCOPY_LOADSTORETOCOPY_H