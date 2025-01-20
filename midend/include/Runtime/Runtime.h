#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include <vector>
#include "Value.h"

namespace mlir {
namespace aegis {

typedef struct tagRuntimeContext {
}RuntimeContext;


class Runtime {
    virtual std::vector<Value> call(const std::vector<Value> &input) = 0;
}



} // namespace aegis
} // namespace mlir

#endif