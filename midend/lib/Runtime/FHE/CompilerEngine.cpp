#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHEPipeline.h"
#include "Runtime/FHE/ProgramSpecGeneration.h"
#include "Common/Error.h"
#include "mlir/Parser/Parser.h"


namespace mlir {
namespace aegis {

/// Creates a new compilation context.
std::shared_ptr<CompileContext> CompileContext::createContext() { return std::make_shared<CompileContext>(); }

/// Returns the MLIR context for a compile context.
mlir::MLIRContext *CompileContext::getMLIRContext() {
    if (this->mlirCtx == nullptr) {
        this->mlirCtx = new mlir::MLIRContext();
    }

    return this->mlirCtx;
}

/// Returns the LLVM context for a compile context.
llvm::LLVMContext *CompileContext::getLLVMContext() {
    if (this->llvmCtx == nullptr)
        this->llvmCtx = new llvm::LLVMContext();

    return this->llvmCtx;
}

llvm::Expected<CompileResult> CompileContext::compile(mlir::ModuleOp module, TARGET target) {
    CompileResult res;
    CompileOptions &options = this->compileOptions;
    mlir::MLIRContext &mlirContext = *this->getMLIRContext();

    // The current compiler does not yet support GPU.
    if (options.beType == BACKEND_TYPE::GPU) {
        return ErrorMsg("The compiler does not yes sopport GPUs.");
    }

    // higher mlir(using onnx-mlir conver module to mlir)
    if (target == TARGET::MLIR) {
        return res;
    }

    // Lower high mlir to low mlir.(eg: unroll affine.for ...)
    if (aegis::fhepipeline::lowerHighLevelMlir(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower higher level mlir.");
    }
    if (target == TARGET::LOWER_MLIR) {
        return res;
    }

    // Lower build-in mlir to Secret IR
    if (aegis::fhepipeline::lowerMlirToSecret(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower buildin mlir to secret ir.");
    }
    if (target == TARGET::SECRET) {
        return res;
    }

    // Lower secret ir to fhe ir
    if (aegis::fhepipeline::lowerSecretToFhe(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower secret ir to fhe ir.");
    }
    if (target == TARGET::FHE) {
        return res;
    }

    // Lower fhe ir to emitc ir
    if (aegis::fhepipeline::lowerFheToEmitc(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower fhe ir to emitc ir.");
    }
    if (target == TARGET::EMITC) {
        return res;
    }

    // Transform emitc ir to cpp
    if (target == TARGET::CPP) {
        if (aegis::fhepipeline::transformEmitcToCpp(mlirContext, module, res.cppFileName, options.verbose).failed()) {
            return ErrorMsg("Failed to transform emitc to cpp.");
        }
    }

    // Compile cpp to library
    if (target == TARGET::LIBRARY) {
        if (aegis::fhepipeline::emitSharedLibrary(res.cppFileName, res.binFileName).failed()) {
            return ErrorMsg("Failed to compile cpp to share library.");
        }
    }

    // Generate prog_spec file.
    if (target == TARGET::CPP || target == TARGET::LIBRARY) {
        auto progSpecOrErr = createProgramSpec(module);
        if (!progSpecOrErr)
            return progSpecOrErr.takeError();
    }

    return res;
}

llvm::Expected<CompileResult> CompileContext::compile(llvm::SourceMgr &sm, TARGET target) {
    mlir::OwningOpRef<mlir::ModuleOp> mlirModuleRef = mlir::parseSourceFile<mlir::ModuleOp>(sm, this->mlirCtx);
    if (!mlirModuleRef) {
        return ErrorMsg("Could not parse source code.");
    }

    return compile(mlirModuleRef.release(), target);
}

llvm::Expected<CompileResult> CompileContext::compile(llvm::StringRef s, TARGET target) {
    std::unique_ptr<llvm::MemoryBuffer> memBuf = llvm::MemoryBuffer::getMemBuffer(s);
    llvm::SourceMgr sm;
    sm.AddNewSourceBuffer(std::move(memBuf), llvm::SMLoc());
    return this->compile(sm, target);
}

} // namespace aegis
} // namespace mlir