#ifndef COMPILER_ENGINE_H
#define COMPILER_ENGINE_H

#include "capnp/message.h"
#include "protocol.capnp.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Pass/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/SourceMgr.h"

namespace mlir {
namespace aegis {

/// Backend type
enum BACKEND_TYPE {
CPU,
GPU,
};

/// Compile Mode
enum COMPILE_MODE {
  COMPILE,
  TRANSPILER,
};

/// Specification of the exit stage of the compilation pipeline
enum class TARGET {
  /// Read sources and convert mlir operations to SECRET operations
  SECRET,

  /// Read sources and lower all SECRET operations to FHE operations
  FHE,

  /// Read sources and lower all FHE operations to the emitc dialect operations. 
  EMITC,

  /// Read sources and convert emitc dialect to c/cpp source code. 
  CPP, 

  /// compile cpp source code to a futur library
  LIBRARY

};


/// Compilation options 
typedef struct tagCompileOptions {
  BACKEND_TYPE beType;
  COMPILE_MODE mode; 

  tagCompileOptions() {
    beType = BACKEND_TYPE::CPU;
    mode = COMPILE_MODE::COMPILE;
  }
} CompileOptions;


/// Result of Compile
class CompileResult {
  std::string outputDirPath;
  std::string cppFileName;
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

public:
  CompileResult compile(mlir::ModuleOp module, TARGET target);
  CompileResult compile(llvm::SourceMgr &sm, TARGET target);
  CompileResult compile(llvm::StringRef s, TARGET target);

protected:
  mlir::MLIRContext *mlirCtx;
  llvm::LLVMContext *llvmCtx;
};

} // namespace aegis
} // namespace mlir

#endif