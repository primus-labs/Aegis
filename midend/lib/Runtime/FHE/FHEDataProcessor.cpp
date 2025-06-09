#include "Runtime/FHE/FHEDataProcessor.h"
#include "Common/ProgramSpec.h"
#include "../../backend/cpu/FHE/include/Operate.h"

#include <numeric> // for std::accumulate

namespace mlir {
namespace aegis {

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

Value FHEDataProcessor::publicInput(const std::vector<double> &theArg) {
    std::vector<uint8_t> bufArg;
    size_t byte_size = theArg.size() * sizeof(double);
    bufArg.resize(byte_size);
    memcpy(bufArg.data(), theArg.data(), byte_size);
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

std::vector<Value> FHEDataProcessor::privateInput(std::vector<Value> &args) {
    std::vector<Value> inputData;
    for (auto& arg : args) {
        inputData.push_back(privateInput(arg));
    }
    
    return inputData;
}

std::vector<Value> FHEDataProcessor::publicInput(std::vector<Value> &args) {
    std::vector<Value> inputData;
    for (auto& arg : args) {
        inputData.push_back(publicInput(arg));
    }

    return inputData;
}

std::vector<Value> FHEDataProcessor::processOutput(std::vector<Value> &outputs) {
    std::vector<Value> outputData;
    for (auto& output : outputs) {
        outputData.push_back(processOutput(output));
    }

    return outputData;
}

Value FHEDataProcessor::privateInput(Value &arg) {
    auto tensor = arg.getTensor<double>().value();
    std::vector<uint8_t> res = aegiscpu::openfhe::encrypt(tensor.values);
    Tensor<uint8_t> resBuf(res, arg.getDims());
    return Value(resBuf);
}

Value FHEDataProcessor::publicInput(Value &arg) {
    return arg;
}

Value FHEDataProcessor::processOutput(Value &output) {
    auto tensor = output.getTensor<uint8_t>().value();
    auto dims = output.getDims();
    std::vector<double> res;
    if (dims.size() == 1) {
        // size_t plaintextSize = std::accumulate(dims.begin(), dims.end(), (size_t)1, std::multiplies<size_t>());
        std::vector<double> res = aegiscpu::openfhe::decrypt(tensor.values, dims[0]);
        
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