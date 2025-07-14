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
    std::vector<Value> outputData;
    for (auto const& output : outputs) {
        outputData.emplace_back(processOutput(output));
    }

    return outputData;
}

Value SimDataProcessor::processOutput(const Value &output) {
    auto tensor = output.getTensor<uint8_t>().value();
    auto dims = output.getDims();
    std::vector<double> res;
    if (dims.size() == 1) {
        if (dims[0] == 1) {
            res.resize(1);
            res[0] = deserializeToDouble(tensor.values);
        } else {
            res = deserializeToVectorDouble(tensor.values);
        }   
    } else if (dims.size() == 2) {
        res = deserializeToVectorDouble(tensor.values);
    } else {
        assert((dims.size() == 1 || dims.size() == 2) && "Invalid dimension: must be 1 or 2");
    }

    return Value(Tensor<double>(res, output.getDims()));
}



} // namespace aegis
} // namespace mlir