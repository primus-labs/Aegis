#include "Runtime/CompilerEngine.h"


namespace mlir {
namespace aegis {

/// Creates a new compilation context.
std::shared_ptr<CompileContext> CompileContext::createContext() {
  return std::make_shared<CompileContext>();
}


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


CompileResult CompileContext::compile(mlir::ModuleOp module, TARGET target) {
  //TODO
  return CompileResult();
}


CompileResult CompileContext::compile(llvm::SourceMgr &sm, TARGET target) {
  //TODO
  return CompileResult();
}


CompileResult CompileContext::compile(llvm::StringRef s, TARGET target) {
  //TODO
  return CompileResult();
}


} // namespace aegis
} // namespace mlir