#include <iostream>

#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/Secret/SecretDialect.h"
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
#include "Pass/Branch/LowerBranch.h"
#include "Pass/LoadStoreToCopy/LoadStoreToCopy.h"
#include "Pass/LweToRlwe/LowerLweToRlwe.h"
#include "Pass/AutoBootstrap/AutoBootstrap.h"
#include "Pass/Unroll/UnrollLoops.h"
#include "Pass/UnrollAndMemOpt/UnrollLoopAndMemOpt.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace aegis;
using namespace secret;
using namespace fhe;

void fhePipeline(OpPassManager &manager) {
    manager.addPass(std::make_unique<CollectMetadataPass>()); // this pass must run first.
    manager.addPass(std::make_unique<ExtractLoopBodyPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<ExpandMemrefCopyPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<UnrollLoopAndMemOptPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<BranchPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(affine::createSimplifyAffineStructuresPass());
    manager.addPass(createLowerAffinePass());
    manager.addPass(createCanonicalizerPass()); 
    manager.addPass(createCSEPass());
    // manager.addPass(std::make_unique<UnrollLoopsPass>());
    // manager.addPass(createCanonicalizerPass()); 
    // manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<GlobalMemrefReplacePass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<ForwardInsertToExtractPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<ForwardStoreToLoadPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LowerArithToSecretPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LowerFuncToSecretPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LowerMemrefToSecretPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LowerSecretToFhePass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<FoldArithChainPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<BatchingPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LoadStoreToCopyPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LweToRlwePass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<AutoBootstrapPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LowerFheToEmitcPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LowerCastToEmitcStubPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<LowerCastToEmitcStubPass>()); // LowerCastToEmitcStubPass must run more times.
    manager.addPass(std::make_unique<InsertEmitcPreamblePass>());
}

void mpcPipeline(OpPassManager &manager) { 
    llvm::errs() << "MPC pipeline is currently not supported.\n"; 
}

void zkpPipeline(OpPassManager &manager) { 
    llvm::errs() << "ZKP pipeline is currently not supported.\n"; 
}

int main(int argc, char **argv) {
    mlir::MLIRContext context;
    context.enableMultithreading();

    mlir::DialectRegistry registry;
    registry.insert<secret::SecretDialect>();
    registry.insert<fhe::FHEDialect>();
    registry.insert<func::FuncDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<tensor::TensorDialect>();
    registry.insert<arith::ArithDialect>();
    registry.insert<emitc::EmitCDialect>();
    registry.insert<func::FuncDialect>();
    registry.insert<linalg::LinalgDialect>();

    context.loadDialect<secret::SecretDialect>();
    context.loadDialect<fhe::FHEDialect>();
    context.loadDialect<func::FuncDialect>();
    context.loadDialect<affine::AffineDialect>();
    context.loadDialect<tensor::TensorDialect>();
    context.loadDialect<arith::ArithDialect>();
    context.loadDialect<emitc::EmitCDialect>();
    context.loadDialect<func::FuncDialect>();
    context.loadDialect<linalg::LinalgDialect>();
    // Uncomment the following to include *all* MLIR Core dialects, or
    // selectively include what you need like above. You only need to register
    // dialects that will be *parsed* by the tool, not the one generated
    registerAllDialects(registry);
    context.loadAllAvailableDialects();

    // Uncomment the following to make *all* MLIR core passes available.
    // This is only useful for experimenting with the command line to compose
    registerAllPasses();

    registerCanonicalizerPass();
    affine::registerAffineLoopUnrollPass();
    PassRegistration<UnrollLoopAndMemOptPass>();
    PassRegistration<BranchPass>();
    PassRegistration<UnrollLoopsPass>();
    PassRegistration<GlobalMemrefReplacePass>();
    PassRegistration<ExtractLoopBodyPass>();
    PassRegistration<ExpandMemrefCopyPass>();
    PassRegistration<ForwardInsertToExtractPass>();
    PassRegistration<ForwardStoreToLoadPass>();
    PassRegistration<LowerArithToSecretPass>();
    PassRegistration<LowerFuncToSecretPass>();
    PassRegistration<LowerMemrefToSecretPass>();
    PassRegistration<CollectMetadataPass>();
    PassRegistration<LowerSecretToFhePass>();
    PassRegistration<FoldArithChainPass>();
    PassRegistration<BatchingPass>();
    PassRegistration<LoadStoreToCopyPass>();
    PassRegistration<LweToRlwePass>();
    PassRegistration<AutoBootstrapPass>();
    PassRegistration<LowerFheToEmitcPass>();
    PassRegistration<LowerCastToEmitcStubPass>();
    PassRegistration<InsertEmitcPreamblePass>();

    PassPipelineRegistration<>("mlir-to-fhe", "Converts standard MLIR operations to FHE operations", fhePipeline);
    PassPipelineRegistration<>("mlir-to-mpc", "Converts standard MLIR operations to MPC operations", mpcPipeline);
    PassPipelineRegistration<>("mlir-to-zkp", "Converts standard MLIR operations to ZKP operations", zkpPipeline);

    return asMainReturnCode(MlirOptMain(argc, argv, "AEGIS optimizer\n", registry));
}
