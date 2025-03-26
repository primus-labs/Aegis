#ifndef PASS_LWE2RLWE_LOWERLWETORLWE_H
#define PASS_LWE2RLWE_LOWERLWETORLWE_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

struct LweToRlwePass : public mlir::PassWrapper<LweToRlwePass, mlir::OperationPass<mlir::ModuleOp>> {
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final { 
        return "lwe-to-rlwe"; 
    }
};

#endif // PASS_LWE2RLWE_LOWERLWETORLWE_H