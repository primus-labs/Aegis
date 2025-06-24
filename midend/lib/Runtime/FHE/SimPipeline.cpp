#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Common/Utils.h"
#include "Runtime/FHE/SimPipeline.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/Dialect/Affine/Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassOptions.h"
#include "mlir/InitAllPasses.h"
#include "mlir/IR/AsmState.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormatVariadic.h"

namespace mlir {
namespace aegis {
namespace simpipeline {

static mlir::LogicalResult moduleOpToString(mlir::ModuleOp &moduleOp, std::string &mlirContent) {
    mlir::MLIRContext *context = moduleOp.getContext();
    context->getOrLoadDialect<mlir::func::FuncDialect>();
    context->getOrLoadDialect<mlir::affine::AffineDialect>();
    context->getOrLoadDialect<mlir::linalg::LinalgDialect>();
    context->getOrLoadDialect<mlir::tensor::TensorDialect>();
    context->getOrLoadDialect<mlir::memref::MemRefDialect>();
    context->getOrLoadDialect<mlir::arith::ArithDialect>();

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


static mlir::LogicalResult fromHighLevelMlirToLowerLevelMlir(const std::string& mlirContent, const std::string &mlirFullFileName) {
    // Find aegiscompiler tool path
    std::string aegisCompileTool = aegis::findAegisTool("aegiscompiler", "AEGIS_COMPILER_PATH");
    if (aegisCompileTool.empty()) {
        llvm::errs() << "aegisCompiler not found in PATH or AEGIS_COMPILER_PATH.\n";
        return failure();
    }
    // const auto &ErrorOrPath = llvm::sys::findProgramByName("aegiscompiler");
    // if (!ErrorOrPath) {
    //     llvm::errs() << "failed to find `aegiscompiler` on the PATH.\n";
    //     return failure();
    // }
    // aegisCompileTool = ErrorOrPath.get();

    // Create a temporary input file
    char inputTemp[] = "/tmp/lower_XXXXXX.mlir";
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

    // exec aegiscompiler tool
    const std::string outputPath(mlirFullFileName);
    pid_t pid = fork();
    if (pid == -1) {
        unlink(inputPath.c_str());
        unlink(outputPath.c_str());
        llvm::errs() << "Failed to fork process.\n";
        return failure();
    }

    if (pid == 0) {
        const char* args[] = {
            aegisCompileTool.c_str(),
            "--collect-metadata",
            "--one-shot-bufferize",
            "--convert-linalg-to-loops",
            "--convert-scf-to-cf",
            "--expand-strided-metadata",
            "--lower-affine",
            "--convert-arith-to-llvm",
            "--convert-scf-to-cf",
            "--finalize-memref-to-llvm",
            "--convert-func-to-llvm",
            "--convert-cf-to-llvm",
            "--reconcile-unrealized-casts",
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
            llvm::errs() << "aegisCompiler execution failed.\n";
            return failure();
        }
    }

    return success();
}


mlir::LogicalResult lowerToLowLevelMLIR(mlir::ModuleOp &moduleOp, const std::string &mlirFullFileName) {
    // Get high level mlir text content.
    std::string mlirContent;
    if (moduleOpToString(moduleOp, mlirContent).failed()) {
        return failure();
    }

    // Exec aegiscompile tool to generate low level mlir file
    return fromHighLevelMlirToLowerLevelMlir(mlirContent, mlirFullFileName);
}

} // namespace simpipeline
} // namespace aegis
} // namespace mlir