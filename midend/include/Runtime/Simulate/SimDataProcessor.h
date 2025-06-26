#ifndef RUNTIME_SIMDATAPROCESSOR_H
#define RUNTIME_SIMDATAPROCESSOR_H

#include "../DataProcessor.h"

namespace mlir {
namespace aegis {


class SimDataProcessor: public DataProcessor {
public:
    Value privateInput(double theArg) override;
    Value privateInput(const std::vector<double> &theArg) override;
    std::vector<Value> privateInput(const std::vector<std::vector<double>> &args) override;
    Value publicInput(double theArg) override;
    Value publicInput(const std::vector<double> &theArg) override;
    std::vector<Value> publicInput(const std::vector<std::vector<double>> &args) override;
    std::vector<Value> processOutput(const std::vector<Value> &outputs) override;
};

} // namespace aegis
} // namespace mlir

#endif //RUNTIME_SIMDATAPROCESSOR_H
