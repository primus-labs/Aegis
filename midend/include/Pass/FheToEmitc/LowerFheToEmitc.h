#ifndef PASS_FHETOEMITC_LOWERFHETOEMITC_H
#define PASS_FHETOEMITC_LOWERFHETOEMITC_H


#include "mlir/Pass/Pass.h"


struct LowerFheToEmitcPass : public mlir::PassWrapper<LowerFheToEmitcPass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "fhe-to-emitc";
    }
};


#endif //PASS_FHETOEMITC_LOWERFHETOEMITC_H