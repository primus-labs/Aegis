#ifndef PASS_FORWARDINSERTTOEXTREACT_FORWARDINSERTTOEXTRACT_H
#define PASS_FORWARDINSERTTOEXTREACT_FORWARDINSERTTOEXTRACT_H

#include "mlir/include/mlir/IR/BuiltinAttributes.h"   
#include "mlir/include/mlir/IR/BuiltinOps.h"          
#include "mlir/include/mlir/IR/BuiltinTypes.h"        
#include "mlir/include/mlir/IR/DialectRegistry.h"
#include "mlir/Pass/Pass.h"


struct ForwardInsertToExtractPass : public mlir::PassWrapper<ForwardInsertToExtractPass, mlir::OperationPass<mlir::ModuleOp>>
{
    //void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "forward-insert-to-extract";
    }
};



#endif //PASS_FORWARDINSERTTOEXTREACT_FORWARDINSERTTOEXTRACT_H