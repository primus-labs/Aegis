#include <iostream>
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
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
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Pass/UnrollAndMemOpt/UnrollLoopAndMemOpt.h"
#include "Pass/Unroll/UnrollLoops.h"
#include "Pass/GlobalMemrefReplace/GlobalMemrefReplace.h"
#include "Pass/ExpandMemrefCopy/ExpandMemrefCopy.h"
#include "Pass/ForwardInsertToExtract/ForwardInsertToExtract.h"
#include "Pass/ForwardStoreToLoad/ForwardStoreToLoad.h"
#include "Pass/ArithToSecret/LowerArithToSecret.h"


using namespace mlir;
using namespace aegis;
using namespace secret;
using namespace fhe;


void fhePipeline(OpPassManager &manager)
{
    manager.addPass(std::make_unique<ExpandMemrefCopyPass>());
    manager.addPass(createCanonicalizerPass());
    manager.addPass(createCSEPass()); 
    manager.addPass(std::make_unique<UnrollLoopAndMemOptPass>());
    manager.addPass(createCanonicalizerPass()); 
    manager.addPass(createCSEPass());
    manager.addPass(std::make_unique<UnrollLoopsPass>());
    manager.addPass(createCanonicalizerPass()); // this can greatly reduce the number of operations after unrolling
    manager.addPass(createCSEPass());
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
}


int main(int argc, char **argv)
{
    mlir::MLIRContext context;
    context.enableMultithreading();

    mlir::DialectRegistry registry;
    registry.insert<SecretDialect>();
    registry.insert<FHEDialect>();
    registry.insert<func::FuncDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<tensor::TensorDialect>();
    registry.insert<arith::ArithDialect>();
    registry.insert<emitc::EmitCDialect>();
    registry.insert<func::FuncDialect>();
    registry.insert<linalg::LinalgDialect>();

    context.loadDialect<SecretDialect>();
    context.loadDialect<FHEDialect>();
    context.loadDialect<func::FuncDialect>();
    context.loadDialect<affine::AffineDialect>();
    context.loadDialect<tensor::TensorDialect>();
    context.loadDialect<arith::ArithDialect>();
    context.loadDialect<emitc::EmitCDialect>();
    context.loadDialect<func::FuncDialect>();
    context.loadDialect<linalg::LinalgDialect>();
    // Uncomment the following to include *all* MLIR Core dialects, or selectively
    // include what you need like above. You only need to register dialects that
    // will be *parsed* by the tool, not the one generated
    registerAllDialects(registry);
    context.loadAllAvailableDialects();

    // Uncomment the following to make *all* MLIR core passes available.
    // This is only useful for experimenting with the command line to compose
    registerAllPasses();

    registerCanonicalizerPass();
    affine::registerAffineLoopUnrollPass();
    registerCSEPass();
    PassRegistration<UnrollLoopAndMemOptPass>();
    PassRegistration<UnrollLoopsPass>();
    PassRegistration<GlobalMemrefReplacePass>();
    PassRegistration<ExpandMemrefCopyPass>();
    PassRegistration<ForwardInsertToExtractPass>();
    PassRegistration<ForwardStoreToLoadPass>();
    PassRegistration<LowerArithToSecretPass>();

    PassPipelineRegistration<>("fhe-pass", "Run fhe-level passes", fhePipeline);

    return asMainReturnCode(MlirOptMain(argc, argv, "AEGIS optimizer\n", registry));
}
