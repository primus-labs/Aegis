#ifndef RUNTIME_DATAPROCESSOR_H
#define RUNTIME_DATAPROCESSOR_H

#include <vector>
#include "../Common/Value.h"

namespace mlir {
namespace aegis {


class DataProcessor {
public:
    virtual std::vector<Value> privateInput(std::vector<Value> &args) = 0;
    virtual std::vector<Value> publicInput(std::vector<Value> &args) = 0;
    virtual std::vector<Value> processOutput(std::vector<Value> &outputs, std::vector<size_t>& plaintextSizes) = 0;
};

} // namespace aegis
} // namespace mlir

#endif