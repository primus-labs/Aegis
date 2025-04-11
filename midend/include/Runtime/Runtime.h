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
    virtual llvm::Expected<bool> open(const std::string &sharedLibPath) = 0;
    virtual llvm::Expected<bool> resolveSymbol(const std::string &funcName) = 0;
    virtual llvm::Expected<std::vector<Value>> call(const std::vector<Value> &input) = 0;
};



} // namespace aegis
} // namespace mlir

#endif