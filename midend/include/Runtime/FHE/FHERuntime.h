#ifndef RUNTIME_FHERUNTIME_H
#define RUNTIME_FHERUNTIME_H

#include <dlfcn.h>
#include "../Runtime.h"
#include "openfhe.h"
using namespace lbcrypto;

namespace mlir {
namespace aegis {


// Argument type enum
enum class ArgType {
    CipherType,
    PlainType,
};


// Argument wrapper base class
struct ArgWrapperBase {
    virtual ~ArgWrapperBase() = default;
    virtual ArgType type() const = 0;
    virtual void* ptr() = 0;
};


// Ciphertext type wrapper
struct CiphertextWrapper : ArgWrapperBase {
    Ciphertext<DCRTPoly> argValue;
    
    CiphertextWrapper(Ciphertext<DCRTPoly> val) : argValue(std::move(val)) {}
    ArgType type() const override { return ArgType::CipherType; }
    void* ptr() override { return static_cast<void*>(&argValue); }
};


// Plaintext type wrapper 
struct PlaintextWrapper : ArgWrapperBase {
    Plaintext argValue;
    
    PlaintextWrapper(Plaintext val) : argValue(std::move(val)) {}
    ArgType type() const override { return ArgType::PlainType; }
    void* ptr() override { return static_cast<void*>(&argValue); }
};


// function invoker core
class FuncInvoker {
public:
    template<typename Ret, typename... Args>
    static Ret invoke(void* funcPtr, const std::vector<ArgWrapperBase*>& args) {
        // check arg numbers
        if (args.size() != sizeof...(Args)) {
            throw std::runtime_error("Number of parameters does not match.");
        }

        // Type checking and unpacking parameters
        return invokeImpl<Ret, Args...>(funcPtr, args, std::index_sequence_for<Args...>{});
    }

private:
    template<typename Ret, typename... Args, size_t... Is>
    static Ret invokeImpl(void* funcPtr, 
                         const std::vector<ArgWrapperBase*>& args,
                         std::index_sequence<Is...>) {
        // Expand parameter pack and perform type checking
        checkTypes<Args...>(args, std::make_index_sequence<sizeof...(Args)>{});

        // Get the actual function pointer
        auto func = reinterpret_cast<Ret(*)(Args...)>(funcPtr);

        // Unpack the parameters and call the function
        return func(*reinterpret_cast<typename std::decay<Args>::type*>(args[Is]->ptr())...);
    }

    template<typename... Ts, size_t... Is>
    static void checkTypes(const std::vector<ArgWrapperBase*>& args, std::index_sequence<Is...>) {
        (checkType<Ts>(args[Is], Is), ...);
    }

    template<typename T>
    static void checkType(ArgWrapperBase* arg, size_t index) {
        constexpr bool isCipher = std::is_same_v<T, Ciphertext<DCRTPoly>>;
        constexpr bool isPlain = std::is_same_v<T, Plaintext>;
        
        static_assert(isCipher || isPlain, "Unsupported parameter type");

        const auto expectedType = isCipher ? ArgType::CipherType 
                                            : ArgType::PlainType;
        
        if (arg->type() != expectedType) {
            throw std::runtime_error("parameter " + std::to_string(index) + " type does not match.");
        }
    }
};


class FHERuntime : public Runtime {
public:
    FHERuntime(const std::string &progSpecFileName)    {
        assert(!progSpecFileName.empty());
        this->progSpecFileName = progSpecFileName;
        libHandle = nullptr;
        funcPtr = nullptr;
    }
    virtual ~FHERuntime() {
        if (libHandle != nullptr) {
            dlclose(libHandle);
            libHandle = nullptr;
            funcPtr = nullptr;
        }
    }

public:
    llvm::Expected<bool> open(const std::string &sharedLibPath) override;
    llvm::Expected<bool> resolveSymbol(const std::string &funcName) override;
    llvm::Expected<std::vector<Value>> call(const std::vector<Value> &input) override;

private:
    void *libHandle;
    void* funcPtr;
    std::string progSpecFileName;
};


} // namespace aegis
} // namespace mlir

#endif