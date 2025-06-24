#ifndef COMPILER_ENGINE_H
#define COMPILER_ENGINE_H

#include "capnp/message.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Pass/Pass.h"
#include "protocol.capnp.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/SourceMgr.h"
#include "Common/Protocol.h"

namespace mlir {
namespace aegis {

/// Backend type
enum class BACKEND_TYPE {
    CPU,
    GPU,
};

/// Fhe scheme
enum class FHE_SCHEME_TYPE {
    BGV, 
    BFV, 
    CKKS
};

/// Specification of the exit stage of the compilation pipeline
enum class TARGET {
    /// Dump all build-in mlir operations
    MLIR,

    /// Read high mlir operations and conver to low mlir operations
    LOWER_MLIR,

    /// Read sources and convert mlir operations to SECRET operations
    SECRET,

    /// Read sources and lower all SECRET operations to FHE operations
    FHE,

    /// Read sources and lower all FHE operations to the emitc dialect operations.
    EMITC,

    /// Read sources and convert emitc dialect to c/cpp source code.
    CPP,

    /// compile cpp source code to a futur library
    LIBRARY,

    /// compiler mlir to lowe lever mlir for simulation
    SIM_MLIR,

};

/// Compilation options
typedef struct tagCompileOptions {
    BACKEND_TYPE beType;
    TARGET target;
    FHE_SCHEME_TYPE scheme;
    bool verbose;
    std::string outputDir;

    tagCompileOptions() {
        beType = BACKEND_TYPE::CPU;
        target = TARGET::LIBRARY;
        scheme = FHE_SCHEME_TYPE::CKKS;
        verbose = false;
        outputDir = "/tmp/aegis/";
    }
} CompileOptions;

/// Result of Compile
struct CompileResult {
    std::string outputDirPath;
    std::string cppFileName;
    std::string simFileName;
    std::string binFileName;
    std::string progSpecFileName;
};

/// Compilation context , indirectly referenced LLVM and MLIR data structures.
class CompileContext {
public:
    CompileContext();
    ~CompileContext();

    mlir::MLIRContext *getMLIRContext();
    llvm::LLVMContext *getLLVMContext();

    static std::shared_ptr<CompileContext> createContext();

protected:
    mlir::MLIRContext *mlirCtx;
    llvm::LLVMContext *llvmCtx;
};


class CompilerEngine {
public:
    CompilerEngine(std::shared_ptr<CompileContext> complileCtx) 
    : compileContext(complileCtx), compileOptions(),
      enablePass([](mlir::Pass *pass) { return true; }) {}

public:
    llvm::Expected<CompileResult> compile(mlir::ModuleOp module);
    llvm::Expected<CompileResult> compile(llvm::SourceMgr &sm);
    llvm::Expected<CompileResult> compile(llvm::StringRef code);

public:
    void setEnablePass(std::function<bool(mlir::Pass *)> enablePass) {
      this->enablePass = enablePass;
    }
    void setCompileOptions(CompileOptions options) {
        compileOptions = options;
    }
    CompileOptions getCompileOptions() { 
        return compileOptions; 
    }

private:
    llvm::Expected<bool> emitSharedLib(const std::string &fullSrcCodeFileName, 
                                    const std::string &fullSharedFileName);
    llvm::Expected<bool> emitProgragSpecToJson(const std::string &fullProgSpecFileName, 
                                ProtoMessage<aegisprotocol::ProgSpec> progSpec);


protected:
    std::shared_ptr<CompileContext> compileContext;
    CompileOptions compileOptions;
    std::function<bool(mlir::Pass *)> enablePass;
};

} // namespace aegis
} // namespace mlir

#endif