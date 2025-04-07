#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include <vector>
#include "../Common/Value.h"

namespace mlir {
namespace aegis {

typedef struct tagRuntimeContext {
}RuntimeContext;


class Runtime {
    virtual bool open(const std::string &sharedLibPath) = 0;
    virtual bool load(const std::string &sharedLibPath, const std::string &funcName) = 0;
    virtual std::vector<Value> call(const std::vector<Value> &input) = 0;
};



} // namespace aegis
} // namespace mlir

#endif