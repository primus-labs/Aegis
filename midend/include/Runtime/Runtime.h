#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include <vector>
#include "llvm/Support/Error.h"
#include "../Common/Value.h"

namespace mlir {
namespace aegis {

typedef struct tagRuntimeContext {
}RuntimeContext;


class Runtime {
    // Loads a shared library (e.g., .so) from the specified path to prepare for subsequent operations.
    virtual llvm::Expected<bool> open(const std::string &sharedLibPath) = 0;

    // Loads cryptographic resources required for homomorphic encryption operations, including various keys.
    virtual llvm::Expected<bool> loadCryptoResources(const std::string &pubKeyFileName, 
                                                     const std::string &multKeyFileName,
                                                     const std::string &rotKeyFileName) = 0;
    
    // Locates and prepares a target function in the loaded shared library for execution.
    virtual llvm::Expected<bool> resolveSymbol(const std::string &funcName) = 0;

    // Executes the resolved cryptographic function with encrypted/plaintext inputs and returns results.
    virtual llvm::Expected<std::vector<Value>> call(const std::vector<Value> &input) = 0;
};



} // namespace aegis
} // namespace mlir

#endif