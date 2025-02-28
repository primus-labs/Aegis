#ifndef PASS_UNROLLANDMEMOPT_UNROLLLOOPANDMEMOPT_H
#define PASS_UNROLLANDMEMOPT_UNROLLLOOPANDMEMOPT_H


#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/IR/BuiltinTypes.h"           
#include "mlir/include/mlir/IR/DialectRegistry.h"  


/// This MLIR pass performs loop unrolling and store-to-load forwarding to optimize memory access patterns in a function.
struct UnrollLoopAndMemOptPass : public mlir::PassWrapper<UnrollLoopAndMemOptPass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "unroll-loop-and-memory-opt";
    }
};


#endif //PASS_UNROLLANDMEMOPT_UNROLLLOOPANDMEMOPT_H