#include "Runtime/FHE/FHEDataProcessor.h"
#include "Common/ProgramSpec.h"
#include "../../backend/cpu/FHE/include/Operate.h"


namespace mlir {
namespace aegis {

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
    std::vector<uint8_t> res = aegiscpu::encrypt(tensor.values);
    Tensor<uint8_t> resBuf(res, arg.getDims());
    return Value(resBuf);
}

Value FHEDataProcessor::publicInput(Value &arg) {
    return arg;
}

Value FHEDataProcessor::processOutput(Value &output) {
    return output;
}

} // namespace aegis
} // namespace mlir