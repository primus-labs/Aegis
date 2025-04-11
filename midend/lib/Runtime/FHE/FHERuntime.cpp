#include "Runtime/FHE/FHERuntime.h"
#include "Common/Error.h"
#include "Common/ProgramSpec.h"
#include "Common/Protocol.h"



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

        // Prepare parameters
        std::vector<std::unique_ptr<ArgWrapperBase>> args;
        
        // TODO: Assume the first parameter is Ciphertext and the second parameter is Plaintext
        Ciphertext<DCRTPoly> c1;
        Plaintext p1; 
        args.emplace_back(new CiphertextWrapper(std::move(c1)));
        args.emplace_back(new PlaintextWrapper(std::move(p1)));

        // Call the function (explicitly specify return type and parameter types)
        Ciphertext<DCRTPoly> result = FuncInvoker::invoke<Ciphertext<DCRTPoly>, Ciphertext<DCRTPoly>, Plaintext>(
                                                    funcPtr, {args[0].get(), args[1].get()});

        // TODO: we must deserial result to Value
        return std::vector<Value> {};
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

llvm::Expected<bool> FHERuntime::resolveSymbol(const std::string &funcName) {
    assert(!funcName.empty());
    funcPtr = dlsym(libHandle, funcName.c_str());
    if (auto error = dlerror()) {
        return ErrorMsg("Circuit symbol not found in dynamic module: " + std::string(error));
    }

    return true;
}

} // namespace aegis
} // namespace mlir