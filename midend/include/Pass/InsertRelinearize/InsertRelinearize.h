#ifndef PASS_INSERTRELINEARIZE_INSERTRELINEARIZE_H
#define PASS_INSERTRELINEARIZE_INSERTRELINEARIZE_H

#include "mlir/Pass/Pass.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/IR/DialectRegistry.h"

/// This MLIR pass automatically inserts ​relinearization operations (e.g., RelinearizeOp) into the program's IR.
struct InsertRelinearizePass : public mlir::PassWrapper<InsertRelinearizePass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "insert-relinearize"; 
    }
};

#endif // PASS_INSERTRELINEARIZE_INSERTRELINEARIZE_H