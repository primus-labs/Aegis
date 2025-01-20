#ifndef RUNTIME_DATAPROCESSOR_H
#define RUNTIME_DATAPROCESSOR_H

#include <vector>
#include "Value.h"

namespace mlir {
namespace aegis {


class DataProcessor {
public:
    virtual std::vector<Value> privateInput(std::vector<Value> &arg) = 0;
    virtual std::vector<Value> publicInput(std::vector<Value> &arg) = 0;
    virtual std::vector<Value> processOutput(std::vector<Value> &output) = 0;
};

} // namespace aegis
} // namespace mlir

#endif