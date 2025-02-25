#ifndef PASS_FORWARDSTORETOLOAD_FORWARDSTORETOLOAD_H
#define PASS_FORWARDSTORETOLOAD_FORWARDSTORETOLOAD_H

#include "mlir/include/mlir/IR/BuiltinAttributes.h"   
#include "mlir/include/mlir/IR/BuiltinOps.h"          
#include "mlir/include/mlir/IR/BuiltinTypes.h"        
#include "mlir/include/mlir/IR/DialectRegistry.h"
#include "mlir/Pass/Pass.h"


struct ForwardStoreToLoadPass : public mlir::PassWrapper<ForwardStoreToLoadPass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "forward-store-to-load";
    }
};

#endif //PASS_FORWARDSTORETOLOAD_FORWARDSTORETOLOAD_H