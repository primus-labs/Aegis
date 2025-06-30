#include "Runtime/Simulate/SimDataProcessor.h"
#include "Common/DoubleSerializer.h"
#include <stdexcept>

namespace mlir {
namespace aegis {


Value SimDataProcessor::privateInput(double theArg) {
    throw std::logic_error("privateInput not supported in SimDataProcessor");
}

Value SimDataProcessor::privateInput(const std::vector<double> &theArg) {
    throw std::logic_error("privateInput not supported in SimDataProcessor");
}

std::vector<Value> SimDataProcessor::privateInput(const std::vector<std::vector<double>> &args) {
    throw std::logic_error("privateInput not supported in SimDataProcessor");
}

Value SimDataProcessor::publicInput(double theArg) {
    std::vector<uint8_t> bufArg = aegis::serializeFormDouble(theArg);
    Value inputVal(Tensor<uint8_t>(bufArg, std::vector<size_t>{0}));
    return inputVal;
}

Value SimDataProcessor::publicInput(const std::vector<double> &theArg) {
    std::vector<uint8_t> bufArg = aegis::serializeFormVectorDouble(theArg);
    Value inputVal(Tensor<uint8_t>(bufArg, std::vector<size_t>{theArg.size()}));
    return inputVal;
}

std::vector<Value> SimDataProcessor::publicInput(const std::vector<std::vector<double>> &theArg) {
    std::vector<uint8_t> bufArg = aegis::serializeFormMatrixDouble(theArg);
    Value inputVal(Tensor<uint8_t>(bufArg, std::vector<size_t>{theArg.size(), theArg[0].size()}));
    return std::vector<Value>{inputVal};
}

std::vector<Value> SimDataProcessor::processOutput(const std::vector<Value> &outputs) {
    //​​Return it directly without any processing
    return outputs;
}



} // namespace aegis
} // namespace mlir