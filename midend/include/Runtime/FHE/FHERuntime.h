#ifndef RUNTIME_FHERUNTIME_H
#define RUNTIME_FHERUNTIME_H

#include <dlfcn.h>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "../Runtime.h"
#include "openfhe.h"
using namespace lbcrypto;

namespace mlir {
namespace aegis {


// Argument type enum
enum class ArgType {
    VectType,
    MatrixType,
};


// Argument wrapper base class
struct ArgWrapperBase {
    virtual ~ArgWrapperBase() = default;
    virtual ArgType type() const = 0;
    virtual void* ptr() = 0;
};


// vector type wrapper
struct VectorWrapper : ArgWrapperBase {
    std::vector<uint8_t> argValue;
    
    VectorWrapper(std::vector<uint8_t> val) : argValue(std::move(val)) {}
    ArgType type() const override { return ArgType::VectType; }
    void* ptr() override { return static_cast<void*>(&argValue); }
};


// matrix type wrapper 
struct MatrixtWrapper : ArgWrapperBase {
    std::vector<std::vector<uint8_t>> argValue;
    
    MatrixtWrapper(std::vector<std::vector<uint8_t>> val) : argValue(std::move(val)) {}
    ArgType type() const override { return ArgType::MatrixType; }
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
        constexpr bool isVector = std::is_same_v<T, std::vector<uint8_t>>;
        constexpr bool isMatrix = std::is_same_v<T, std::vector<std::vector<uint8_t>>>;  
        static_assert(isVector || isMatrix, "Unsupported parameter type");
        const auto expectedType = isVector ? ArgType::VectType : ArgType::MatrixType;
        
        if (arg->type() != expectedType) {
            throw std::runtime_error("parameter " + std::to_string(index) + " type does not match.");
        }
    }
};


// type sequence container
template<ArgType... Types>
struct TypeSequence {
    template<ArgType NewType>
    using Push = TypeSequence<NewType, Types...>; // Adding new types to the sequence frontend
};

// Generates a sequence of types from high to low bits.
// If N=3, Mask=5 (binary 101), TypeSequence<vector, matrix, vector> is generated.
template<size_t N, uint32_t Mask>
struct GenerateTypeSequence {
    using Type = typename GenerateTypeSequence<N - 1, Mask>::Type::template Push<
        ((Mask >> (N - 1)) & 0x1) ? ArgType::VectType : ArgType::MatrixType>;
};

// Recursion termination condition
template<uint32_t Mask>
struct GenerateTypeSequence<0, Mask> {
    using Type = TypeSequence<>;
};

// Conversion of type sequences to actual calls
template<typename Ret, typename TypeSeq>
struct InvokeWithTypeSeq;

template<typename Ret, ArgType... Types>
struct InvokeWithTypeSeq<Ret, TypeSequence<Types...>> {
    static Ret call(void* funcPtr, const std::vector<ArgWrapperBase*>& args) {
        return FuncInvoker::invoke<Ret,
            std::conditional_t<Types == ArgType::VectType,
                              std::vector<uint8_t>,
                              std::vector<std::vector<uint8_t>>>...>(funcPtr, args);
    }
};

// Recursively check the type and call the corresponding Mask branch
template<typename Ret, size_t N, uint32_t Mask, uint32_t TotalMasks>
struct DynamicInvokerMaskBranch {
    static Ret checkAndInvoke(void* funcPtr, const std::vector<ArgWrapperBase*>& args) {
        //Start checking from bit 0
        if (checkArgTypes<Mask, N, 0>(args)) { 
            return InvokeWithTypeSeq<Ret, typename GenerateTypeSequence<N, Mask>::Type>::call(funcPtr, args);
        }
        // Otherwise try the next Mask
        return DynamicInvokerMaskBranch<Ret, N, Mask + 1, TotalMasks>::checkAndInvoke(funcPtr, args);
    }

private:
    template<uint32_t CurrentMask, size_t TotalBits, size_t BitPos>
    static bool checkArgTypes(const std::vector<ArgWrapperBase*>& args) {
        if constexpr (BitPos >= TotalBits) {
            return true;
        } else {
            constexpr ArgType expectedType = (CurrentMask & (1 << BitPos)) 
                                           ? ArgType::VectType 
                                           : ArgType::MatrixType;
            if (args[BitPos]->type() != expectedType) {
                return false;
            }
            return checkArgTypes<CurrentMask, TotalBits, BitPos + 1>(args);
        }
    }
};

// Specialized termination condition: throw an exception when Mask >= TotalMasks
template<typename Ret, size_t N, uint32_t TotalMasks>
struct DynamicInvokerMaskBranch<Ret, N, TotalMasks, TotalMasks> {
    static Ret checkAndInvoke(void*, const std::vector<ArgWrapperBase*>&) {
        throw std::runtime_error("No matching type combination");
    }
};

// Dynamically calling the executor
template<typename Ret, size_t N>
struct DynamicInvoker {
    static Ret invoke(void* funcPtr, const std::vector<ArgWrapperBase*>& args) {
        constexpr uint32_t TotalMasks = 1 << N;
        return DynamicInvokerMaskBranch<Ret, N, 0, TotalMasks>::checkAndInvoke(funcPtr, args);
    }
};

template<typename Ret>
Ret dispatchInvoke(void* funcPtr, const std::vector<ArgWrapperBase*>& args) {
    const size_t n = args.size();
    switch(n) {
        case 0: return DynamicInvoker<Ret, 0>::invoke(funcPtr, args);
        case 1: return DynamicInvoker<Ret, 1>::invoke(funcPtr, args);
        case 2: return DynamicInvoker<Ret, 2>::invoke(funcPtr, args);
        case 3: return DynamicInvoker<Ret, 3>::invoke(funcPtr, args);
        case 4: return DynamicInvoker<Ret, 4>::invoke(funcPtr, args);
        case 5: return DynamicInvoker<Ret, 5>::invoke(funcPtr, args);
        default: throw std::runtime_error("Unsupported parameter count");
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
    llvm::Expected<bool> loadCryptoResources(const std::string &cryptCtxFileName,
                                             const std::string &pubKeyFileName, 
                                             const std::string &multKeyFileName, 
                                             const std::string &rotKeyFileName) override;
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