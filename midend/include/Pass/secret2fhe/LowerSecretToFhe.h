#ifndef PASS_SECRET2FHE_LOWERSECRETTOFHE_H
#define PASS_SECRET2FHE_LOWERSECRETTOFHE_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/FHE/FHEDialect.h"


struct LowerSecretToFhePass : public mlir::PassWrapper<LowerSecretToFhePass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "secret2fhe";
    }
};


#endif //PASS_SECRET2FHE_LOWERSECRETTOFHE_H
