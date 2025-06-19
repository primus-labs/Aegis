#ifndef RUNTIME_FHEDATAPROCESSOR_H
#define RUNTIME_FHEDATAPROCESSOR_H

#include "../DataProcessor.h"

namespace mlir {
namespace aegis {


class FHEDataProcessor: public DataProcessor {
public:
    Value privateInput(double theArg) override;
    Value privateInput(const std::vector<double> &theArg) override;
    std::vector<Value> privateInput(const std::vector<std::vector<double>> &args) override;
    Value publicInput(double theArg) override;
    Value publicInput(const std::vector<double> &theArg) override;
    std::vector<Value> publicInput(const std::vector<std::vector<double>> &args) override;
    std::vector<Value> processOutput(const std::vector<Value> &outputs) override;


private:
    Value processOutput(const Value &output);
};

} // namespace aegis
} // namespace mlir

#endif