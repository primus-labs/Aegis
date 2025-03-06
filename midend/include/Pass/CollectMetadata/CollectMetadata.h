#ifndef PASS_COLLECTMETADATA_COLLECTMETADEATA_H
#define PASS_COLLECTMETADATA_COLLECTMETADEATA_H

#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

/// Collect and store all metadata information, primarily function parameter attributes, 
/// for use in subsequent passes.
struct CollectMetadataPass : public mlir::PassWrapper<CollectMetadataPass, mlir::OperationPass<mlir::ModuleOp>>
{
    // This never generates new kinds of operations that weren't previously in the program => no dependent dialects

    void runOnOperation() override;

    mlir::StringRef getArgument() const final
    {
        return "collect-metadata";
    }

private:
    void collectAllMetadata(mlir::func::FuncOp funcOp);
};

#endif //PASS_COLLECTMETADATA_COLLECTMETADEATA_H