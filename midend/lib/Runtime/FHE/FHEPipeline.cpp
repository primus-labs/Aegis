#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Runtime/FHE/FHEPipeline.h"
#include "Common/Utils.h"
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
#include "Pass/MultiDimLoad/LowerMultiDimLoad.h"
#include "Pass/Batching/Batching.h"
#include "Pass/LoadStoreToCopy/LoadStoreToCopy.h"
#include "Pass/LweToRlwe/LowerLweToRlwe.h"
#include "Pass/AutoBootstrap/AutoBootstrap.h"
#include "Pass/UnrollAndMemOpt/UnrollLoopAndMemOpt.h"
#include "Pass/Branch/LowerBranch.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassOptions.h"
#include "mlir/InitAllPasses.h"
#include "mlir/IR/AsmState.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/FileSystem.h"

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

static mlir::LogicalResult emitcOpToString(mlir::ModuleOp &moduleOp, std::string &mlirContent) {
    mlir::MLIRContext *context = moduleOp.getContext();
    context->getOrLoadDialect<mlir::emitc::EmitCDialect>();

    mlir::OpPrintingFlags printFlags;
    printFlags.enableDebugInfo();  
    printFlags.elideLargeElementsAttrs(10); 

    mlirContent.clear();
    llvm::raw_string_ostream sos(mlirContent);
    moduleOp.print(sos, printFlags);
    
    if (mlirContent.empty()) {
        llvm::errs() << "Failed to convert ModuleOp to mlir text.\n";
        return failure();
    }
    
    return success();
}

static mlir::LogicalResult fromEmitcToCpp(const std::string& mlirContent, const std::string &cppFullFileName) {
    // Find emitc-translate tool path
    std::string emitcTranTool = aegis::findAegisTool("mlir-translate", "MLIR_TRANSLATE_PATH");
    if (emitcTranTool.empty()) {
        llvm::errs() << "mlir-translate not found in PATH or MLIR_TRANSLATE_PATH.\n";
        return failure();
    }

    // Write mlir content to input file
    std::string tmpFileName = cppFullFileName + ".tmp";
    std::ofstream inputFile(tmpFileName);
    if (!inputFile) {
        llvm::errs() << "Cannot open input file: " << tmpFileName << ".\n";
        return failure();
    }
    inputFile << mlirContent;
    inputFile.close();

    // Exec emitc-translate tool to get result
    std::vector<std::string> argsTrans = {
        "--mlir-to-cpp",
        tmpFileName,
        "-o",
        cppFullFileName,
    };
    std::string transRetContent, errContent;
    if (executeAegisTool(emitcTranTool, argsTrans, transRetContent, errContent)) {
        llvm::errs() << errContent << "\n";
        return failure();
    }

    // Write return content to cpp file
    // std::ofstream outFile(cppFullFileName);
    // if (!outFile) {
    //     llvm::errs() << "Cannot open input file: " << cppFullFileName << ".\n";
    //     return failure();
    // }
    // outFile << transRetContent;

    llvm::sys::fs::remove(tmpFileName);
    return success();
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
    addNestedAwarePass(pm, std::make_unique<ExpandMemrefCopyPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<UnrollLoopAndMemOptPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<BranchPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, affine::createSimplifyAffineStructuresPass(), enablePass);
    addNestedAwarePass(pm, createLowerAffinePass(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<GlobalMemrefReplacePass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<ForwardInsertToExtractPass>(), enablePass);
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
    addNestedAwarePass(pm, std::make_unique<LowerMultiDimLoadPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<BatchingPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<LoadStoreToCopyPass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<LweToRlwePass>(), enablePass);
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<AutoBootstrapPass>(), enablePass);
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
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
    addNestedAwarePass(pm, std::make_unique<LowerCastToEmitcStubPass>(), enablePass); // LowerCastToEmitcStubPass must run more times.

    return pm.run(module.getOperation());
}


mlir::LogicalResult lowerEmitcFinalize(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                    std::function<bool(mlir::Pass *)> enablePass, 
                                    const std::string &progSpecFileName, bool verbose) {
    mlir::PassManager pm(&context);
    printPipeline("lowerEmitcFinalize", pm, context, verbose);

    addNestedAwarePass(pm, std::make_unique<InsertEmitcPreamblePass>(progSpecFileName), enablePass);
    return pm.run(module.getOperation());
}


mlir::LogicalResult transformEmitcToCpp(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                        const std::string &cppFullFileName, bool verbose) {
    mlir::PassManager pm(&context);
    printPipeline("transformEmitcToCpp", pm, context, verbose);

    // Get emitc mlir text content.
    std::string mlirContent;
    if (emitcOpToString(module, mlirContent).failed()) {
        return failure();
    }

    // Exec emit-translate tool to generate cpp file
    return fromEmitcToCpp(mlirContent, cppFullFileName);
}



} // namespace fhepipeline
} // namespace aegis
} // namespace mlir