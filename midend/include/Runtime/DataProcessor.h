#ifndef RUNTIME_DATAPROCESSOR_H
#define RUNTIME_DATAPROCESSOR_H

#include <vector>
#include "../Common/Value.h"

namespace mlir {
namespace aegis {


class DataProcessor {
public:
    virtual Value privateInput(const std::vector<double> &theArg) = 0;
    virtual std::vector<Value> privateInput(const std::vector<std::vector<double>> &args) = 0;
    virtual Value publicInput(const std::vector<double> &theArg) = 0;
    virtual std::vector<Value> publicInput(const std::vector<std::vector<double>> &args) = 0;
    virtual std::vector<Value> processOutput(const std::vector<Value> &outputs) = 0;
};

} // namespace aegis
} // namespace mlir

#endif