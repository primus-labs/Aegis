#ifndef PASS_EXPANDMEMREFCOPY_EXPANDMEMREFCOPY_H
#define PASS_EXPANDMEMREFCOPY_EXPANDMEMREFCOPY_H

#include "mlir/include/mlir/IR/BuiltinAttributes.h"   
#include "mlir/include/mlir/IR/BuiltinOps.h"          
#include "mlir/include/mlir/IR/BuiltinTypes.h"        
#include "mlir/include/mlir/IR/DialectRegistry.h"
#include "mlir/Pass/Pass.h"


struct ExpandMemrefCopyPass : public mlir::PassWrapper<ExpandMemrefCopyPass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "expand-memref-copy";
    }
};


#endif //PASS_EXPANDMEMREFCOPY_EXPANDMEMREFCOPY_H