#ifndef PASS_UNROLL_UNROLLLOOPS_H_
#define PASS_UNROLL_UNROLLLOOPS_H_

#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

/// Aggressively tries to unroll all affine.for (affine::AffineForOp) loops
struct UnrollLoopsPass : public mlir::PassWrapper<UnrollLoopsPass, mlir::OperationPass<mlir::ModuleOp>>
{
    // This never generates new kinds of operations that weren't previously in the program => no dependent dialects

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "unroll-loops";
    }
};

#endif // PASS_UNROLL_UNROLLLOOPS_H_