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
        int valIdx = 0;
        std::vector<ArgWrapperBase*> args;
        for (auto func : vectFuncs) {
            for (auto arg : func.asReader().getInputs()) {
                // Get the input argument dims
                auto dims = arg.getShape().getDimensions();
                auto dim_size = dims.size();
                assert(dim_size <= 0 && "Function parameter dimensions are incorrect");
                assert(dim_size > 2 && "Function parameters with dimensions higher than 2D are currently not supported");

                if (arg.getType()) {
                    if (dim_size == 1) {
                        // Each Value in the vector container input represents a parameter (a Ciphertext object)
                        std::vector<uint8_t> cipherData = input[valIdx++].getTensor<uint8_t>().value().values;
                        args.emplace_back(new VectorWrapper(cipherData));
                    } else if (dim_size > 1){
                        // Multi Value in the vector container input represents a parameter (a Ciphertext object)
                        std::vector<std::vector<uint8_t>> cipherData;
                        for (auto i = 0; i < dims[0]; i++) {
                            std::vector<uint8_t> unitCipherData = input[valIdx++].getTensor<uint8_t>().value().values;
                            cipherData.emplace_back(unitCipherData);
                        }
                        args.emplace_back(new MatrixtWrapper(cipherData));
                    }
                } else {
                    if (dim_size == 1) {
                        // clear argument, Each Value in the vector container input represents a parameter
                        // Not a Plaintext object, but standard types such as uint_8 and other integer values.
                        std::vector<uint8_t> plainData = input[valIdx++].getTensor<uint8_t>().value().values;
                        args.emplace_back(new VectorWrapper(plainData));
                    } else if (dim_size > 1) {
                        // clear argument, Multi Value in the vector container input represents a parameter
                        // Not a Plaintext object, but standard types such as uint_8 and other integer values.
                        std::vector<std::vector<uint8_t>> plainData;
                        for (auto i = 0; i < dims[0]; i++) {
                            std::vector<uint8_t> unitPlainData = input[valIdx++].getTensor<uint8_t>().value().values;
                            plainData.emplace_back(unitPlainData);
                        }
                        args.emplace_back(new MatrixtWrapper(plainData));
                    }
                }
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
        return ErrorMsg(e.what());
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