#ifndef PASS_INSEERTEMITCPREAMBLE_INSERTEMITCPREAMBLE_H
#define PASS_INSEERTEMITCPREAMBLE_INSERTEMITCPREAMBLE_H


#include "mlir/Pass/Pass.h"   
#include "mlir/include/mlir/IR/DialectRegistry.h"  


/// This MLIR pass inserts emitc preamble code (include directives and verbatim C++ text) 
/// immediately before the first occurrence of a ModelOp within an MLIR module.
/// This pass is useful for injecting boilerplate code (e.g., headers, namespaces, or macros) required by downstream C++ code generation.
struct InsertEmitcPreamblePass : public mlir::PassWrapper<InsertEmitcPreamblePass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "insert-emitc-preamble";
    }
};


#endif //PASS_INSEERTEMITCPREAMBLE_INSERTEMITCPREAMBLE_H