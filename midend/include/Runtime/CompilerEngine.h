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


/// Compilation options 
struct CompileOptions {
};


/// Result of Compile
class CompileResult {
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

} // namespace aegis
} // namespace mlir

#endif