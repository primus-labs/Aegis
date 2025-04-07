#include "Runtime/FHE/FHERuntime.h"
#include "Common/Error.h"


namespace mlir {
namespace aegis {

std::vector<Value> FHERuntime::call(const std::vector<Value> &input) {
    try {
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
        ErrorMsg err;
        err << "error:" << e.what() << "\n";
        return std::vector<Value> {};
    }
}

bool FHERuntime::open(const std::string &sharedLibPath) {
    libHandle = dlopen(sharedLibPath.c_str(), RTLD_LAZY);
    if (!libHandle) {
        ErrorMsg err;
        err << "Cannot open shared library " << dlerror();
        return false;
    }

    return true;
}

bool FHERuntime::load(const std::string &sharedLibPath, const std::string &funcName) {
    assert(!sharedLibPath.empty());
    assert(!funcName.empty());
    funcPtr = dlsym(libHandle, funcName.c_str());
    if (auto error = dlerror()) {
        ErrorMsg err;
        err << "Circuit symbol not found in dynamic module: "
            << std::string(error);
        return false;
    }

    return true;
}

} // namespace aegis
} // namespace mlir