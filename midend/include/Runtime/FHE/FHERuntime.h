#ifndef RUNTIME_FHERUNTIME_H
#define RUNTIME_FHERUNTIME_H

#include "../Runtime.h"

namespace mlir {
namespace aegis {


class FHERuntime : public Runtime {
public:
    std::vector<Value> call(const std::vector<Value> &input);

};


} // namespace aegis
} // namespace mlir

#endif