#ifndef PASS_CASTTOEMITCSTUB_LOWERCASTTOEMITCSTUB_H
#define PASS_CASTTOEMITCSTUB_LOWERCASTTOEMITCSTUB_H


#include "mlir/Pass/Pass.h"         
#include "mlir/include/mlir/IR/DialectRegistry.h"  


/// This pass lowers cast operations to a dummy emitc function call. 
/// Since the original cast operation does not conform to EmitCType, 
/// it cannot be directly converted to emitc::CastOp. Instead, 
/// this pass replaces it with an emitc::CallOpaqueOp, which serves as a placeholder or no-op function in the EmitC dialect.
/// Note: This pass must be run at the end of the lowering pipeline, specifically after FheToEmitc, 
/// to ensure that all other conversions are completed before handling unsupported cast operations.
struct LowerCastToEmitcStubPass : public mlir::PassWrapper<LowerCastToEmitcStubPass, mlir::OperationPass<mlir::ModuleOp>>
{
    void getDependentDialects(mlir::DialectRegistry &registry) const override;

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "cast-to-emitc-stub";
    }
};


#endif //PASS_CASTTOEMITCSTUB_LOWERCASTTOEMITCSTUB_H