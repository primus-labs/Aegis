#include "Runtime/FHE/FHEDataProcessor.h"
#include "Common/ProgramSpec.h"
#include "Common/DoubleSerializer.h"
#include "../../backend/cpu/FHE/include/Operate.h"
#include <numeric> 

namespace mlir {
namespace aegis {

Value FHEDataProcessor::privateInput(double theArg) {
    std::vector<uint8_t> bufArg = aegiscpu::openfhe::encrypt(std::vector<double>(theArg));
    Value inputVal(Tensor<uint8_t>(bufArg, std::vector<size_t>{1}));
    return inputVal;
}

Value FHEDataProcessor::privateInput(const std::vector<double> &theArg) {
    std::vector<uint8_t> bufArg = aegiscpu::openfhe::encrypt(theArg);
    Value inputVal(Tensor<uint8_t>(bufArg, std::vector<size_t>{theArg.size()}));
    return inputVal;
}

std::vector<Value> FHEDataProcessor::privateInput(const std::vector<std::vector<double>> &args) {
    std::vector<Value> inputData;
    for (auto i = 0; i < args.size(); i++) {
        auto theInput = privateInput(args[i]);
        inputData.emplace_back(theInput);
    }

    return inputData;
}

Value FHEDataProcessor::publicInput(double theArg) {
    std::vector<uint8_t> bufArg = aegis::serializeFormDouble(theArg);
    Value inputVal(Tensor<uint8_t>(bufArg, std::vector<size_t>{1}));
    return inputVal;
}

Value FHEDataProcessor::publicInput(const std::vector<double> &theArg) {
    std::vector<uint8_t> bufArg = aegis::serializeFormVectorDouble(theArg);
    Value inputVal(Tensor<uint8_t>(bufArg, std::vector<size_t>{theArg.size()}));
    return inputVal;
}

std::vector<Value> FHEDataProcessor::publicInput(const std::vector<std::vector<double>> &args) {
    std::vector<Value> inputData;
    for (auto i = 0; i < args.size(); i++) {
        auto theInput = publicInput(args[i]);
        inputData.emplace_back(theInput);
    }

    return inputData;
}

std::vector<Value> FHEDataProcessor::processOutput(const std::vector<Value> &outputs) {
    std::vector<Value> outputData;
    for (auto const& output : outputs) {
        outputData.emplace_back(processOutput(output));
    }

    return outputData;
}

Value FHEDataProcessor::processOutput(const Value &output) {
    auto tensor = output.getTensor<uint8_t>().value();
    auto dims = output.getDims();
    std::vector<double> res;
    if (dims.size() == 1) {
        // size_t plaintextSize = std::accumulate(dims.begin(), dims.end(), (size_t)1, std::multiplies<size_t>());
        res = aegiscpu::openfhe::decrypt(tensor.values, dims[0]);
        
    } else if (dims.size() == 2) {
        std::vector<std::vector<double>> decVal = aegiscpu::openfhe::decryptBatch(tensor.values, dims[0], dims[1]);
        for (auto i = 0; i < decVal.size(); i++) {
            res.insert(res.end(), decVal[i].begin(), decVal[i].end());
        }
    } else {
        assert((dims.size() == 1 || dims.size() == 2) && "Invalid dimension: must be 1 or 2");
    }

    return Value(Tensor<double>(res, output.getDims()));
}

} // namespace aegis
} // namespace mlir