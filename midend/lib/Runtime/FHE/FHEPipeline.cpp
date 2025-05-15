#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

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
#include "Pass/LoadStoreToCopy/LoadStoreToCopy.h"
#include "Pass/LweToRlwe/LowerLweToRlwe.h"
#include "Pass/AutoBootstrap/AutoBootstrap.h"
#include "Pass/Unroll/UnrollLoops.h"
#include "Pass/UnrollAndMemOpt/UnrollLoopAndMemOpt.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassOptions.h"
#include "mlir/InitAllPasses.h"
#include "mlir/IR/AsmState.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "llvm/Support/FormatVariadic.h"

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

static bool findEmitcTranslateTool(std::string &toolPath) {
    toolPath.clear();

    // Check environment variables
    if (const char* env_path = std::getenv("MLIR_TRANSLATE_PATH")) {
        struct stat statbuf;
        if (stat(env_path, &statbuf) == 0 && (statbuf.st_mode & S_IXUSR)) {
            toolPath = std::string(llvm::formatv("{0}/mlir-translate", env_path));
            return true;
        }
    }

    // Search PATH environment variable
    const char* path_env = std::getenv("PATH");
    if (!path_env) {
        return false;
    }

    std::vector<std::string> search_paths;
    const std::string delimiter = ":";
    std::string path_str(path_env);
    size_t pos = 0;
    while ((pos = path_str.find(delimiter)) != std::string::npos) {
        search_paths.push_back(path_str.substr(0, pos));
        path_str.erase(0, pos + delimiter.length());
    }
    search_paths.push_back(path_str);

    // Traverse search paths
    for (const auto& dir : search_paths) {
        std::string full_path = dir + "/mlir-translate";
        struct stat statbuf;
        if (stat(full_path.c_str(), &statbuf) == 0 && (statbuf.st_mode & S_IXUSR)) {
            toolPath = full_path;
            return true;
        }
    }

    return false;
}

static mlir::LogicalResult moduleOpToString(mlir::ModuleOp &moduleOp, std::string &mlirContent) {
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
    std::string emitcTranTool;
    if (!findEmitcTranslateTool(emitcTranTool)) {
        llvm::errs() << "mlir-translate not found in PATH or MLIR_TRANSLATE_PATH.\n";
        return failure();
    }

    // Create a temporary input file
    char inputTemp[] = "/tmp/emitc_XXXXXX.mlir";
    int fdInput = mkstemps(inputTemp, 5); 
    if (fdInput == -1) {
        llvm::errs() << "Failed to create temporary input file.\n";
        return failure();
    }
    close(fdInput);
    
    const std::string inputPath(inputTemp);
    {
        std::ofstream inputFile(inputPath);
        if (!inputFile) {
            llvm::errs() << "Cannot open input file: " << inputPath << ".\n";
            return failure();
        }
        inputFile << mlirContent;
    }

    // exec mlir-translate tool
    const std::string outputPath(cppFullFileName);
    pid_t pid = fork();
    if (pid == -1) {
        unlink(inputPath.c_str());
        unlink(outputPath.c_str());
        llvm::errs() << "Failed to fork process.\n";
        return failure();
    }

    if (pid == 0) {
        const char* args[] = {
            emitcTranTool.c_str(),
            "--mlir-to-cpp",
            inputPath.c_str(),
            "-o", outputPath.c_str(),
            nullptr
        };

        execvp(args[0], const_cast<char* const*>(args));
        exit(EXIT_FAILURE);
    } else { 
        int status;
        waitpid(pid, &status, 0);
        unlink(inputPath.c_str());

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            unlink(outputPath.c_str());
            llvm::errs() << "mlir-translate execution failed.\n";
            return failure();
        }
    }

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
    addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    addNestedAwarePass(pm, createCSEPass(), enablePass);
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
    // addNestedAwarePass(pm, std::make_unique<FoldArithChainPass>(), enablePass);
    // addNestedAwarePass(pm, createCanonicalizerPass(), enablePass);
    // addNestedAwarePass(pm, createCSEPass(), enablePass);
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
    if (moduleOpToString(module, mlirContent).failed()) {
        return failure();
    }

    // Exec emit-translate tool to generate cpp file
    return fromEmitcToCpp(mlirContent, cppFullFileName);
}



} // namespace fhepipeline
} // namespace aegis
} // namespace mlir