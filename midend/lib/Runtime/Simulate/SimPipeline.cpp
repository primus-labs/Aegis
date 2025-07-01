#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Common/Utils.h"
#include "Runtime/Simulate/SimPipeline.h"
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
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
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

static void extractFuncPrototypeInfo(mlir::ModuleOp &moduleOp, std::string &funcName, 
                                     std::vector<std::vector<int64_t>> &argDims, 
                                     std::vector<std::vector<int64_t>> &retDims) {
    auto getParamOrRetDims = [](func::FuncOp funcOp, bool isArguments, std::vector<std::vector<int64_t>> &dims) -> void {
        const size_t numArgs = isArguments ? funcOp.getNumArguments() : funcOp.getNumResults();
        for (unsigned i = 0; i < numArgs; ++i) {
            Type elementType =
                isArguments ? funcOp.getFunctionType().getInput(i) : funcOp.getFunctionType().getResult(i);

            // Process shaped types
            std::vector<int64_t> itemDims;
            if (auto shapedType = mlir::dyn_cast<ShapedType>(elementType)) {
                if (shapedType.hasStaticShape()) {
                    for (int64_t dim : shapedType.getShape()) {
                        itemDims.push_back(dim);
                    }
                }
            } else {
                // Handle non-shaped types (scalars) by defaulting to dim=1
                itemDims.push_back(1);
            }

            //Store dims
            dims.emplace_back(itemDims);
        }
    };

    moduleOp.walk([&](func::FuncOp funcOp) {
        funcName = funcOp.getName().str();
        getParamOrRetDims(funcOp, true, argDims);
        getParamOrRetDims(funcOp, false, retDims);

        return mlir::WalkResult::interrupt();
    });
}

static mlir::LogicalResult getEntryPointFuncStmts(mlir::ModuleOp &moduleOp, std::string &entryPointStmts) {
    // Get function prototype
    std::string funcName;
    std::vector<std::vector<int64_t>> argDims, retDims;
    extractFuncPrototypeInfo(moduleOp, funcName, argDims, retDims);

    // Get argument statements
    std::string argsStmts;
    std::vector<std::string> argsTys;
    auto argIdx = 0;
    for (auto dims : argDims) {
        std::string unitArgStmt;
        if (!dims.size() || dims[0] == 1) {
            unitArgStmt = std::string(llvm::formatv("%arg{0} = arith.constant ${0}$ : f64\n", argIdx));
            argsTys.push_back("f64");
        } else {
            std::string strDimsTy;
            if (dims.size() == 1) {
                strDimsTy = std::to_string(dims[0]) + "xf64";
            } else if (dims.size() == 2) {
                strDimsTy = std::to_string(dims[0]) + "x" + std::to_string(dims[1]) + "xf64";
            } else {
                assert(false && "Dimensions higher than 2D are currently not supported.");
            }
            unitArgStmt = std::string(llvm::formatv("%arg{0}_tensor = arith.constant dense<${0}$> : tensor<{1}>\n", 
                                      argIdx, strDimsTy));
            auto toMem  = std::string(llvm::formatv("%arg{0} = bufferization.to_memref %arg{0}_tensor : memref<{1}>\n", 
                                      argIdx, strDimsTy));
            unitArgStmt += toMem;

            auto unitArgTy = std::string(llvm::formatv("memref<{0}>", strDimsTy));
            argsTys.emplace_back(unitArgTy);
        }
        argsStmts += unitArgStmt;
        argIdx++; 
    }

    // Get output types
    std::string retTys;
    std::vector<int64_t> dims;
    for (auto dim : retDims[0]) {
        dims.push_back(dim);
    }
    if (!dims.size() || dims[0] == 1) {
        retTys = "f64";
    } else {
        std::string strDimsTy;
        if (dims.size() == 1) {
            strDimsTy = std::to_string(dims[0]) + "xf64";
        } else if (dims.size() == 2) {
            strDimsTy = std::to_string(dims[0]) + "x" + std::to_string(dims[1]) + "xf64";
        } else {
            assert(false && "Dimensions higher than 2D are currently not supported.");
        }
        retTys = std::string(llvm::formatv("memref<{0}>", strDimsTy));
    }

    // Combine function call statements
    std::string callFuncStmts;
    std::string argsNames, combArgsTys;
    for (auto i = 0; i < argsTys.size(); i++) {
        combArgsTys += argsTys[i];
        argsNames += "%arg" + std::to_string(i);
        if (i != (argsTys.size() - 1)) {
            combArgsTys += ',';
            argsNames += ',';
        }
    }

    auto callee = funcName;
    callFuncStmts = std::string(llvm::formatv("%result = func.call @{0}({1}) : ({2}) -> {3}\n", 
                                callee, argsNames, combArgsTys, retTys));


    // Get memref.cast statement
    const std::string real_res = "%real_res";
    std::string castStmt;
    if (!dims.size() || dims[0] == 1) {
        castStmt = std::string(llvm::formatv("%c0 = arith.constant 0 : index\n"
                                             "{0}_tmp = memref.alloca() : memref<1xf64>\n"
                                             "memref.store %result, {0}_tmp[%c0] : memref<1xf64>\n"
                                             "{0} = memref.cast {0}_tmp : memref<1xf64> to memref<*xf64>\n", 
                               real_res));
    } else {
        castStmt = std::string(llvm::formatv("{0} = memref.cast %result : {1} to memref<*xf64>\n", 
                               real_res, retTys));
    }

    // Combine entry point function
    entryPointStmts.clear();
    entryPointStmts = std::string(llvm::formatv(kEntryPointFunc.data(), 
                                  argsStmts, callFuncStmts, castStmt, real_res));

    return success();
}

mlir::LogicalResult appendEntryPointFunc(mlir::ModuleOp &moduleOp, std::string &mlirContent) {
    // Find the position of the last closing brace
    auto lastBracePos = mlirContent.find_last_of('}');
    if (lastBracePos == std::string::npos) {
        llvm::errs() << "No closing brace found in the mlir content \n";
        return failure();
    }
    auto insertPos = lastBracePos;

    // Get entryp point function content
    std::string entryPointStmts;
    if (getEntryPointFuncStmts(moduleOp, entryPointStmts).failed()) {
        return failure();
    }

    // Create the insertion block with proper indentation
    std::string insertionBlock = "\n\n" + entryPointStmts + "\n\n";

    // Insert the code block
    mlirContent.insert(insertPos, insertionBlock);

    return success();
}

mlir::LogicalResult lowerToSimulateMLIR(mlir::ModuleOp &moduleOp, const std::string &mlirFullFileName) {
    // Get high level mlir text content.
    std::string mlirContent;
    if (moduleOpToString(moduleOp, mlirContent).failed()) {
        return failure();
    }

    // Append entry point function to the mlir content
    if (appendEntryPointFunc(moduleOp, mlirContent).failed()) {
        return failure();
    }

    // writer mlir content to mlir file
    if (auto fout = std::ofstream(mlirFullFileName)){
        fout << mlirContent;
        return success();
    } else {
        return failure();
    }
}

} // namespace simpipeline
} // namespace aegis
} // namespace mlir