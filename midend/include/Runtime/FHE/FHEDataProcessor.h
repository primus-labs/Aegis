#ifndef RUNTIME_FHEDATAPROCESSOR_H
#define RUNTIME_FHEDATAPROCESSOR_H

#include "../DataProcessor.h"

namespace mlir {
namespace aegis {


class FHEDataProcessor: public DataProcessor {
public:
    std::vector<Value> privateInput(std::vector<Value> &args) override;
    std::vector<Value> publicInput(std::vector<Value> &args) override;
    std::vector<Value> processOutput(std::vector<Value> &outputs, std::vector<size_t>& plaintextSizes) override;

private:
    Value privateInput(Value &arg);
    Value publicInput(Value &arg);
    Value processOutput(Value &output, size_t plaintextSize);
};

} // namespace aegis
} // namespace mlir

#endif