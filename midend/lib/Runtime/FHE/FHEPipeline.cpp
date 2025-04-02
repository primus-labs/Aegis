#include "Runtime/FHE/FHEPipeline.h"
#include "Pass/ArithToSecret/LowerArithToSecret.h"
#include "Pass/CastToEmitcStub/LowerCastToEmitcStub.h"
#include "Pass/CollectMetadata/CollectMetadata.h"
#include "Pass/ExpandMemrefCopy/ExpandMemrefCopy.h"
#include "Pass/ExtractLoopBody/ExtractLoopBody.h"
#include "Pass/FheToEmitc/LowerFheToEmitc.h"
#include "Pass/ForwardInsertToExtract/ForwardInsertToExtract.h"
#include "Pass/ForwardStoreToLoad/ForwardStoreToLoad.h"
#include "Pass/FuncToSecret/LowerFuncToSecret.h"
#include "Pass/GlobalMemrefReplace/GlobalMemrefReplace.h"
#include "Pass/InsertEmitcPreamble/InsertEmitcPreamble.h"
#include "Pass/MemrefToSecret/LowerMemrefToSecret.h"
#include "Pass/SecretToFhe/LowerSecretToFhe.h"
#include "Pass/FoldArithChain/FoldArithChain.h"
#include "Pass/Batching/Batching.h"
#include "Pass/LweToRlwe/LowerLweToRlwe.h"
#include "Pass/Unroll/UnrollLoops.h"
#include "Pass/UnrollAndMemOpt/UnrollLoopAndMemOpt.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassOptions.h"
#include "mlir/InitAllPasses.h"

namespace mlir {
namespace aegis {
namespace fhepipeline {

static void printPipeline(llvm::StringRef name, mlir::PassManager &pm, mlir::MLIRContext &ctx, bool verbose) {
    if (verbose) {
        llvm::outs() << "*************************************************\n"
                     << "***** pipeline => " << name << " <=\n";
        auto isModule = [](mlir::Pass *, mlir::Operation *op) { 
            return mlir::isa<mlir::ModuleOp>(op); 
        };

        ctx.disableMultithreading(true);
        pm.enableIRPrinting(isModule, isModule);
        pm.enableStatistics();
        pm.enableTiming();
        pm.enableVerifier();
    }
}

static void addNestedAwarePass(mlir::PassManager &pm, std::unique_ptr<Pass> pass,
                               std::function<bool(mlir::Pass *)> enablePass) {
    if (!enablePass(pass.get())) {
        llvm::outs() << "Warning: Disable " << pass.get()->getName() << ".\n";
        return;
    }

    if (!pass->getOpName() || *pass->getOpName() == "builtin.module") {
        pm.addPass(std::move(pass));
    } else {
        mlir::OpPassManager &opm = pm.nest(*pass->getOpName());
        opm.addPass(std::move(pass));
    }
}

mlir::LogicalResult lowerHighLevelMlir(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                       std::function<bool(mlir::Pass *)> enablePass, bool verbose) {
    mlir::PassManager pm(&context);
    printPipeline("lowerHighLevelMlir", pm, context, verbose);

    addNestedAwarePass(pm, std::make_unique<CollectMetadataPass>(), enablePass);
    addNestedAwarePass(pm, std::make_unique<ExtractLoopBodyPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<ExpandMemrefCopyPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<UnrollLoopAndMemOptPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, affine::createSimplifyAffineStructuresPass(), enablePass);
    addNestedAwarePass(pm, createLowerAffinePass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<GlobalMemrefReplacePass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<ForwardInsertToExtractPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<ForwardStoreToLoadPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);

    return pm.run(module.getOperation());
}

mlir::LogicalResult lowerMlirToSecret(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                    std::function<bool(mlir::Pass *)> enablePass, bool verbose) {
    mlir::PassManager pm(&context);
    printPipeline("lowerMlirToSecret", pm, context, verbose);

    addNestedAwarePass(pm, std::make_unique<LowerArithToSecretPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<LowerFuncToSecretPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<LowerMemrefToSecretPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    
    return pm.run(module.getOperation());
}


mlir::LogicalResult lowerSecretToFhe(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                    std::function<bool(mlir::Pass *)> enablePass, bool verbose) {
    mlir::PassManager pm(&context);
    printPipeline("lowerSecretToFhe", pm, context, verbose);

    addNestedAwarePass(pm, std::make_unique<LowerSecretToFhePass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<FoldArithChainPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<BatchingPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<LweToRlwePass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);

    return pm.run(module.getOperation());
}


mlir::LogicalResult lowerFheToEmitc(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                    std::function<bool(mlir::Pass *)> enablePass, bool verbose) {
    mlir::PassManager pm(&context);
    printPipeline("lowerFheToEmitc", pm, context, verbose);

    addNestedAwarePass(pm, std::make_unique<LowerFheToEmitcPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<LowerCastToEmitcStubPass>(), enablePass);
    addNestedAwarePass(pm, std::make_unique<InsertEmitcPreamblePass>(), enablePass);

    return pm.run(module.getOperation());
}


mlir::LogicalResult transformEmitcToCpp(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                        std::function<bool(mlir::Pass *)> enablePass, bool verbose) {
    return success();
}


} // namespace fhepipeline
} // namespace aegis
} // namespace mlir