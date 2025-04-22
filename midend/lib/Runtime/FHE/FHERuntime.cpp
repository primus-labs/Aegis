#include <complex>
#include "Runtime/FHE/FHERuntime.h"
#include "Common/Error.h"
#include "Common/ProgramSpec.h"
#include "Common/Protocol.h"
#include "Common/Value.h"
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

        // Prepare parameters
        int idx = 0;
        std::vector<ArgWrapperBase*> args;
        for (auto func : vectFuncs) {
            for (auto param : func.asReader().getInputs()) {
                if (param.getType()) {
                    Ciphertext<DCRTPoly> ct;
                    Tensor<uint8_t> tensorVal = input[idx].getTensor<uint8_t>().value();
                    ct = aegiscpu::deserializeCiphertext(tensorVal.values);
                    args.emplace_back(new CiphertextWrapper(ct));
                } else {
                    Plaintext pt;
                    std::vector<uint8_t> realData = input[idx].getTensor<uint8_t>().value().values;
                    std::vector<std::complex<double>> complexData;
                    for (uint8_t x : realData) {
                        complexData.emplace_back((double)x, 0.0);
                    }
                    auto cc = aegiscpu::CryptoContextMgr::getInstance().getCryptoContext();
                    pt = cc->MakeCKKSPackedPlaintext(complexData);
                    args.emplace_back(new PlaintextWrapper(pt));
                }
                idx++;
            }
        }

        // Call the function (explicitly specify return type and parameter types)
        Ciphertext<DCRTPoly> result = dispatchInvoke<Ciphertext<DCRTPoly>>(funcPtr, args);

        // Clean up parameter wrapping
        for (auto arg : args) {
            delete arg;
        }
        args.clear();

        // TODO: we must kown the result value dims.
        std::vector<uint8_t> bytes;
        aegiscpu::serializeCiphertext(result, bytes);
        Value res(Tensor<uint8_t>(bytes, std::vector<size_t>{0}));
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

llvm::Expected<bool> FHERuntime::loadCryptoResources(const std::string &cryptCtxFileName, const std::string &pubKeyFileName, 
                                                     const std::string &multKeyFileName, const std::string &rotKeyFileName) {
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

    typedef bool (*InitCryptFunc)(const std::string&, const std::string&, const std::string&, const std::string&);
    InitCryptFunc initCryptCtxPrt = reinterpret_cast<InitCryptFunc>(dlsym(libHandle, "init_cryptcontext"));
    if (auto error = dlerror()) {
        return ErrorMsg("Circuit symbol not found in dynamic module: " + std::string(error));
    }
    assert(initCryptCtxPrt);

    return initCryptCtxPrt(cryptCtxFileName, pubKeyFileName, multKeyFileName, rotKeyFileName);
}

llvm::Expected<bool> FHERuntime::resolveSymbol(const std::string &funcName) {
    assert(libHandle && "open function must be called before calling resolveSymbol");
    assert(!funcName.empty() && "function name must not be empty");
    funcPtr = dlsym(libHandle, funcName.c_str());
    if (auto error = dlerror()) {
        return ErrorMsg("Circuit symbol not found in dynamic module: " + std::string(error));
    }
    assert(funcPtr);

    return true;
}

} // namespace aegis
} // namespace mlir