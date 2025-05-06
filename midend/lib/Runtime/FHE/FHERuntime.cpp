#include <complex>
#include "Runtime/FHE/FHERuntime.h"
#include "Common/Error.h"
#include "Common/ProgramSpec.h"
#include "Common/Protocol.h"
#include "Common/Value.h"
#include "Common/Utils.h"
#include "cpu/FHE/include/Operate.h"
#include "cpu/FHE/include/CryptoContextMgr.h"

namespace mlir {
namespace aegis {

llvm::Expected<std::vector<Value>> FHERuntime::call(const std::vector<Value> &input) {
    try {
        // Get ProtoMessage<aegisprotocol::ProgSpec>
        ProgramSpec &progSpec = ProgramSpec::getInstance();
        if (!progSpec.initialize(progSpecFileName)) {
            return ErrorMsg("call ProgramSpec::initilize failure.");
        }
        ProtoMessage<aegisprotocol::ProgSpec> protoProgSpec = progSpec.getProgSpec();
        std::vector<ProtoMessage<aegisprotocol::Function>> vectFuncs = progSpec.getFuncInfo();
        assert(vectFuncs.size() == 1 && "Only one public function can be generated.");

        // Prepare parameters
        int idx = 0;
        std::vector<ArgWrapperBase*> args;
        for (auto func : vectFuncs) {
            for (auto param : func.asReader().getInputs()) {
                if (param.getType()) {
                    // Ciphertext params, Currently only supports the vector<uint8_t> type,
                    // with plans to extend support to vector<vector<uint8_t>> types in the future.
                    std::vector<uint8_t> realData = input[idx].getTensor<uint8_t>().value().values;
                    args.emplace_back(new VectorWrapper(realData));
                } else {
                    // Plaintext params, it must be the vector<uint8_t> type.
                    std::vector<uint8_t> realData = input[idx].getTensor<uint8_t>().value().values;
                    args.emplace_back(new VectorWrapper(realData));
                }
                idx++;
            }
        }

        // Call the function (explicitly specify return type and parameter types)
        // return type only supports the vector<uint8_t> type
        std::vector<uint8_t> result = dispatchInvoke<std::vector<uint8_t>>(funcPtr, args);

        // Clean up parameter wrapping
        for (auto arg : args) {
            delete arg;
        }
        args.clear();

        // Get the result value dims.
        auto outputShape = vectFuncs[0].asReader().getOutputs()[0].getShape();
        std::vector<size_t> dims;
        for (auto dim : outputShape.getDimensions()) {
            dims.push_back(dim);
        }
        Value res(Tensor<uint8_t>(result, dims));
        return std::vector<Value>{res};
    }
    catch (const std::exception& e) {
        std::string err("error:");
        err += e.what();
        return ErrorMsg(err.c_str());
    }
}

llvm::Expected<bool> FHERuntime::open(const std::string &sharedLibPath) {
    libHandle = dlopen(sharedLibPath.c_str(), RTLD_LAZY);
    if (!libHandle) {
        return ErrorMsg("Cannot open shared library " + std::string(dlerror()));
    }

    return true;
}

llvm::Expected<bool> FHERuntime::loadCryptoResources(const std::string &cryptCtxFileName,
                                                     const std::string &pubKeyFileName, 
                                                     const std::string &multKeyFileName, 
                                                     const std::string &rotKeyFileName) {
    assert(libHandle && "open must be called before calling loadCryptoResources");
    if (cryptCtxFileName.empty()) {
        return ErrorMsg("crypt context serialization file name must not be empty");
    }
    if (pubKeyFileName.empty()) {
        return ErrorMsg("public key serialization file name must not be empty");
    }
    if (multKeyFileName.empty()) {
        return ErrorMsg("mult eva key serialization file name must not be empty");
    }

    typedef bool (*LoadCryptoResFunc)(const std::string&, const std::string&, const std::string&, const std::string&);
    LoadCryptoResFunc loadResPrt = reinterpret_cast<LoadCryptoResFunc>(dlsym(libHandle, "loadCryptoResources"));
    if (auto error = dlerror()) {
        return ErrorMsg("Circuit symbol not found in dynamic module: " + std::string(error));
    }
    assert(loadResPrt);

    return loadResPrt(cryptCtxFileName, pubKeyFileName, multKeyFileName, rotKeyFileName);
}

llvm::Expected<bool> FHERuntime::resolveSymbol(const std::string &funcName) {
    assert(libHandle && "open function must be called before calling resolveSymbol");
    assert(!funcName.empty() && "function name must not be empty");

    std::string mangleFuncName = "aegis_mlir_" + funcName;
    funcPtr = dlsym(libHandle, mangleFuncName.c_str());
    if (auto error = dlerror()) {
        return ErrorMsg("Circuit symbol not found in dynamic module: " + std::string(error));
    }
    assert(funcPtr);

    return true;
}

} // namespace aegis
} // namespace mlir