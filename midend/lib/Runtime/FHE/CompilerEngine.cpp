#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHEPipeline.h"
#include "Runtime/FHE/ProgramSpecGeneration.h"
#include "Common/Error.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "mlir/Parser/Parser.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/include/mlir/Dialect/MemRef/IR/MemRef.h"  
#include "mlir/include/mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/include/mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/include/mlir/Dialect/Arith/IR/Arith.h"   
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/FormatVariadic.h"


namespace mlir {
namespace aegis {

const std::string COMPILER = "g++";
#ifdef __APPLE__
    const std::string LINKER_SHARED_OPT =
        " -dylib -undefined dynamic_lookup -L /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem -o ";
    const std::string SHARED_LIB_EXT = ".dylib";
#else // Linux
    const std::string LINKER_SHARED_OPT = " --shared -fPIC -o ";
    const std::string SHARED_LIB_EXT = ".so";
#endif


/// Creates a new compilation context.
std::shared_ptr<CompileContext> CompileContext::createContext() { return std::make_shared<CompileContext>(); }

/// Returns the MLIR context for a compile context.
mlir::MLIRContext *CompileContext::getMLIRContext() {
    if (this->mlirCtx == nullptr) {
        mlir::DialectRegistry registry;
        registry.insert<
            secret::SecretDialect, fhe::FHEDialect,
            mlir::func::FuncDialect, mlir::linalg::LinalgDialect,
            mlir::arith::ArithDialect, mlir::memref::MemRefDialect, 
            mlir::scf::SCFDialect>();

        this->mlirCtx = new mlir::MLIRContext();
        this->mlirCtx->appendDialectRegistry(registry);
        this->mlirCtx->loadAllAvailableDialects();
        this->mlirCtx->disableMultithreading();
    }

    return this->mlirCtx;
}

/// Returns the LLVM context for a compile context.
llvm::LLVMContext *CompileContext::getLLVMContext() {
    if (this->llvmCtx == nullptr)
        this->llvmCtx = new llvm::LLVMContext();

    return this->llvmCtx;
}

CompileContext::CompileContext() : mlirCtx(nullptr), llvmCtx(nullptr) {}
CompileContext::~CompileContext() {
    delete this->mlirCtx;
    delete this->llvmCtx;
}

llvm::Expected<CompileResult> CompilerEngine::compile(mlir::ModuleOp module) {
    CompileOptions &options = this->compileOptions;
    mlir::MLIRContext &mlirContext = *this->compileContext->getMLIRContext();
    CompileResult res;
    res.outputDirPath = options.outputDir;

    // The current compiler does not yet support GPU.
    if (options.beType == BACKEND_TYPE::GPU) {
        return ErrorMsg("The compiler does not yes sopport GPUs.");
    }

    // higher mlir(using onnx-mlir conver module to mlir)
    if (options.target == TARGET::MLIR) {
        return res;
    }

    // Lower high mlir to low mlir.(eg: unroll affine.for ...)
    if (aegis::fhepipeline::lowerHighLevelMlir(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower higher level mlir.");
    }
    if (options.target == TARGET::LOWER_MLIR) {
        return res;
    }

    // Lower build-in mlir to Secret IR
    if (aegis::fhepipeline::lowerMlirToSecret(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower buildin mlir to secret ir.");
    }
    if (options.target == TARGET::SECRET) {
        return res;
    }

    // Lower secret ir to fhe ir
    if (aegis::fhepipeline::lowerSecretToFhe(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower secret ir to fhe ir.");
    }
    if (options.target == TARGET::FHE) {
        return res;
    }

    // Generate prog_spec file.
    // We must first generate the prog_spec.json file, because in the lowerFheToEmitc pipeline, 
    // we need to generate the corresponding cpp code based on the prog_spec.json.
    std::string fullProgSpecJsonFileName;
    res.progSpecFileName = "prog_spec.json";
    fullProgSpecJsonFileName = res.outputDirPath + '/' + res.progSpecFileName;

    // Recursively create directories (eg: auto-create /tmp/aegis if missing)
    std::error_code error = llvm::sys::fs::create_directories(res.outputDirPath);
    if (error) {
        return ErrorMsg("Directory creation failed: " + error.message());
    }

    // Lower fhe ir to emitc ir
    if (aegis::fhepipeline::lowerFheToEmitc(mlirContext, module, enablePass, options.verbose).failed()) {
        return ErrorMsg("Failed to lower fhe ir to emitc ir.");
    }

    // Generate prog_spec.json
    if (options.target == TARGET::CPP || options.target == TARGET::LIBRARY) {
        auto progSpecOrErr = createProgramSpec(module, options);
        if (!progSpecOrErr) {
            return progSpecOrErr.takeError();
        }

        if (!emitProgragSpecToJson(fullProgSpecJsonFileName, progSpecOrErr.get())) {
            return ErrorMsg("Failed to generate program spec json file.");
        }
    }

    // Lower emitc ir finalize(prepare for codegen)
    if (aegis::fhepipeline::lowerEmitcFinalize(mlirContext, module, enablePass, fullProgSpecJsonFileName, options.verbose).failed()) {
        return ErrorMsg("Failed to lower emitc ir finalize");
    }
    if (options.target == TARGET::EMITC) {
        return res;
    }

    // Transform emitc ir to cpp
    std::string fullCppFileName;
    res.cppFileName = "output.cpp";
    fullCppFileName = res.outputDirPath + '/' + res.cppFileName;
    if (aegis::fhepipeline::transformEmitcToCpp(mlirContext, module, fullCppFileName, options.verbose).failed()) {
        return ErrorMsg("Failed to transform emitc to cpp.");
    }
    if (options.target == TARGET::CPP) {
        return res;
    }
    
    // Compile cpp to library
    std::string fullBinFileName;
    res.binFileName = "libaegisshared" + SHARED_LIB_EXT;
    fullBinFileName = res.outputDirPath + '/' + res.binFileName;
    auto emitShareRes = emitSharedLib(fullCppFileName, fullBinFileName);
    if (!emitShareRes) {
        return ErrorMsg(llvm::toString(emitShareRes.takeError()));
    }

    return res;
}

llvm::Expected<CompileResult> CompilerEngine::compile(llvm::SourceMgr &sm) {
    // Catching errors with ScopedDiagnosticHandler
    std::string errorMsg;
    mlir::SourceMgrDiagnosticHandler sourceMgrHandler(sm, this->compileContext->getMLIRContext(), llvm::errs());

    // Redirect diagnostic information to errorMsg;
    llvm::raw_string_ostream errorStream(errorMsg);
    this->compileContext->getMLIRContext()->getDiagEngine().registerHandler(
        [&](mlir::Diagnostic& diag) -> mlir::LogicalResult {
            if (diag.getSeverity() == mlir::DiagnosticSeverity::Error) {
                errorStream << diag << "\n";
            }
            return mlir::failure();
        }
    );

    // parser source code to moduleOp
    mlir::OwningOpRef<mlir::ModuleOp> mlirModuleRef = mlir::parseSourceFile<mlir::ModuleOp>(sm, 
                                                        this->compileContext->getMLIRContext());

    if (!mlirModuleRef) {
        errorStream.flush();
        return ErrorMsg("Parse failed: " + errorMsg);
    }

    // compile ModuleOp
    return compile(mlirModuleRef.release());
}

llvm::Expected<CompileResult> CompilerEngine::compile(llvm::StringRef code) {
    std::unique_ptr<llvm::MemoryBuffer> memBuf = llvm::MemoryBuffer::getMemBuffer(code);
    llvm::SourceMgr sm;
    sm.AddNewSourceBuffer(std::move(memBuf), llvm::SMLoc());
    return this->compile(sm);
}

llvm::Expected<bool> CompilerEngine::emitSharedLib(const std::string &fullSrcCodeFileName, 
                                                   const std::string &fullSharedFileName) {
    if (fullSrcCodeFileName.empty()) {
        return ErrorMsg("source code file name is empty.");
    }
    if (fullSharedFileName.empty()) {
        return ErrorMsg("shared file name is empty.");
    }

    llvm::ErrorOr<std::string> GppPath = llvm::sys::findProgramByName(COMPILER);
    if (!GppPath) {
        return ErrorMsg(COMPILER + "not found in the system env path.");
    }

    // Combine compiler command.
    // eg: g++ func.cpp -Iopenfhe_install_path --shared -fPIC -o func.so 
    const std::string fheIncPath = " -I/usr/local/include/openfhe"
                                   " -I/usr/local/include/openfhe/core"
                                   " -I/usr/local/include/openfhe/pke"
                                   " -I/usr/local/include/openfhe/binfhe";
    std::string compileCmd = std::string(llvm::formatv("{0} {1} {2} {3} {4}", *GppPath, 
                                fullSrcCodeFileName, fheIncPath, LINKER_SHARED_OPT, fullSharedFileName));

    // Lambda signature: Takes StringRef command, returns llvm::Expected<bool>
    auto execCompileCmd = [](StringRef compileCmd) -> llvm::Expected<bool>  {
        // Parse command line arguments
        SmallVector<StringRef, 8> Args;
        compileCmd.split(Args, ' ', -1, false);

        if (Args.empty()) {
            auto errMsg = std::string(llvm::formatv("Empty compile command:{0}" , compileCmd));
            return ErrorMsg(errMsg);
        }

        // Create temporary file for capturing command output
        SmallString<256> OutputPath;
        std::error_code EC = llvm::sys::fs::createTemporaryFile("aegis-compile-result", "log", OutputPath);
        if (EC) {
            auto errMsg = std::string(llvm::formatv("Failed to create temp file:{0}" , EC.message()));
            return ErrorMsg(errMsg);
        }

        // Configure standard stream redirection:
        // - stdin: Inherit from parent
        // - stdout: Redirect to temporary file
        // - stderr: Inherit from parent
        std::optional<StringRef> Redirects[3] = {
            std::nullopt,   // stdin
            OutputPath,     // stdout
            OutputPath      // stderr
        };


        // Execute child process synchronously
        int ExitCode = llvm::sys::ExecuteAndWait(
            Args[0],        // Executable path
            Args,           // Command arguments
            std::nullopt,   // The program's environment
            Redirects,      // Redirect strings
            0,              // No timeout
            0               // No memory limit
        );

        // Read captured output from temporary file
        std::string OutputContent;
        if (auto Buf = llvm::MemoryBuffer::getFile(OutputPath)) {
            OutputContent = Buf.get()->getBuffer().str();
        } else {
            return ErrorMsg("Failed to read output");
        }

        // Clean up temporary file
        if (llvm::sys::fs::remove(OutputPath)) {
            return ErrorMsg("Failed to remove temp file");
        }

        // Validate exit status
        if (ExitCode != 0) {
            auto errMsg = std::string(llvm::formatv("Command failed, Exit code:{0}, Output log:{1}",
                                                    ExitCode, OutputContent));
            return ErrorMsg(errMsg);
        }

        return true;
    };

    return execCompileCmd(compileCmd);
}

llvm::Expected<bool> CompilerEngine::emitProgragSpecToJson(const std::string &fullProgSpecFileName,
                                                        ProtoMessage<aegisprotocol::ProgSpec> progSpec) {
    std::error_code error;
    llvm::raw_fd_ostream out(fullProgSpecFileName, error, llvm::sys::fs::OF_None);
    if (error) {
        return ErrorMsg("Failed to open file: " + error.message());
    }

    auto jsonContent = progSpec.writeJsonToString();
    if (jsonContent.empty()) {
        return ErrorMsg("call writeJsonToString() failure.");
    }

    out << jsonContent;
    if (out.has_error()) { 
        return ErrorMsg("Failed to write content: " + out.error().message());
    }

    out.close();

    return true;
}


} // namespace aegis
} // namespace mlir