#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHEPipeline.h"
#include "Runtime/FHE/ProgramSpecGeneration.h"
#include "Common/Error.h"
#include "mlir/Parser/Parser.h"


namespace mlir {
namespace aegis {

const std::string COMPILER = "g++";
#ifdef __APPLE__
    const std::string LINKER_SHARED_OPT =
        " -dylib -undefined dynamic_lookup -L /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem -o ";
    const std::string SHARED_LIB_EXT = ".dylib";
#else // Linux
    const std::string LINKER_SHARED_OPT = " --shared -o ";
    const std::string SHARED_LIB_EXT = ".so";
#endif


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

llvm::Expected<CompileResult> CompilerEngine::compile(mlir::ModuleOp module, TARGET target) {
    CompileOptions &options = this->compileOptions;
    mlir::MLIRContext &mlirContext = *this->compileContext->getMLIRContext();
    CompileResult res;
    res.outputDirPath = options.outputDir;

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

    // Generate prog_spec file.
    // We must first generate the prog_spec.json file, because in the lowerFheToEmitc pipeline, 
    // we need to generate the corresponding cpp code based on the prog_spec.json.
    std::string fullProgSpecJsonFileName;
    res.progSpecFileName = "prog_spec.json";
    fullProgSpecJsonFileName = res.outputDirPath + '/' + res.progSpecFileName;
    if (target == TARGET::CPP || target == TARGET::LIBRARY) {
        auto progSpecOrErr = createProgramSpec(module);
        if (!progSpecOrErr) {
            return progSpecOrErr.takeError();
        }

        if (!emitProgragSpecToJson(fullProgSpecJsonFileName, progSpecOrErr.get())) {
            return ErrorMsg("Failed to generate program spec json file.");
        }
    }

    // Lower fhe ir to emitc ir
    if (aegis::fhepipeline::lowerFheToEmitc(mlirContext, module, enablePass, fullProgSpecJsonFileName, options.verbose).failed()) {
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
        if (!emitSharedLib(res.cppFileName, res.outputDirPath, res.binFileName)) {
            return ErrorMsg("Failed to compile cpp to share library.");
        }
    }

    return res;
}

llvm::Expected<CompileResult> CompilerEngine::compile(llvm::SourceMgr &sm, TARGET target) {
    mlir::OwningOpRef<mlir::ModuleOp> mlirModuleRef = mlir::parseSourceFile<mlir::ModuleOp>(sm, 
                                                        this->compileContext->getMLIRContext());
    if (!mlirModuleRef) {
        return ErrorMsg("Could not parse source code.");
    }

    return compile(mlirModuleRef.release(), target);
}

llvm::Expected<CompileResult> CompilerEngine::compile(llvm::StringRef s, TARGET target) {
    std::unique_ptr<llvm::MemoryBuffer> memBuf = llvm::MemoryBuffer::getMemBuffer(s);
    llvm::SourceMgr sm;
    sm.AddNewSourceBuffer(std::move(memBuf), llvm::SMLoc());
    return this->compile(sm, target);
}

llvm::Expected<std::string> CompilerEngine::emitSharedLib(const std::string &fullSrcCodeFileName, 
                                            const std::string &outputDirPath, const std::string &sharedLibName) {
    if (fullSrcCodeFileName.empty()) {
        return ErrorMsg("source code file name is empty.");
    }

    std::string sharedFullLibName(outputDirPath);
    if (!sharedLibName.empty()) {
        sharedFullLibName += sharedLibName + SHARED_LIB_EXT;
    } else {
        sharedFullLibName += "libaegisshared" + SHARED_LIB_EXT;
    }

    // Combine compiler command.
    // eg: g++ func.cpp --shared -o func.so
    std::string compileCmd = COMPILER + fullSrcCodeFileName + LINKER_SHARED_OPT + sharedFullLibName;
    
    errno = 0;
    FILE *fp = popen(compileCmd.c_str(), "r");
    if (NULL == fp) {
        return ErrorMsg(strerror(errno)) << "\nCannot call the compiler command: " << compileCmd;
    }

    std::string outputContent;
    const int CHUNK_SIZE = 1024;
    char chunk[CHUNK_SIZE];

    while (fgets(chunk, CHUNK_SIZE, fp) != NULL) {
        outputContent += chunk;
    }
    int status = pclose(fp);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return sharedFullLibName;
    } else if (status == -1) {
        return ErrorMsg("Cannot pclose: " + compileCmd);
    } else {
        return ErrorMsg("Command failed:" + compileCmd + "\nCode:" + std::to_string(status) + "\n" + outputContent);
    }
}

llvm::Expected<bool> CompilerEngine::emitProgragSpecToJson(const std::string &fullProgSpecFileName,
                                                        ProtoMessage<aegisprotocol::ProgSpec> progSpec) {
    std::error_code error;
    llvm::raw_fd_ostream out(fullProgSpecFileName, error);
    auto jsonContent = progSpec.writeJsonToString();
    if (jsonContent.empty()) {
        return ErrorMsg("call writeJsonToString() failure.");
    }
    out << jsonContent;
    out.close();

    return true;
}


} // namespace aegis
} // namespace mlir