#ifndef RUNTIME_FHEDATAPROCESSOR_H
#define RUNTIME_FHEDATAPROCESSOR_H

#include "../DataProcessor.h"

namespace mlir {
namespace aegis {


class FHEDataProcessor: public DataProcessor {
public:
    Value privateInput(const std::vector<double> &theArg) override;
    std::vector<Value> privateInput(const std::vector<std::vector<double>> &args) override;
    std::vector<Value> privateInput(std::vector<Value> &args) override;
    Value publicInput(const std::vector<double> &theArg) override;
    std::vector<Value> publicInput(const std::vector<std::vector<double>> &args) override;
    std::vector<Value> publicInput(std::vector<Value> &args) override;
    std::vector<Value> processOutput(std::vector<Value> &outputs) override;


private:
    Value privateInput(Value &arg);
    Value publicInput(Value &arg);
    Value processOutput(Value &output);
};

} // namespace aegis
} // namespace mlir

#endif