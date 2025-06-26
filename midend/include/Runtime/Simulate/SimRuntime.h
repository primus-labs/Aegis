#ifndef RUNTIME_SIMRUNTIME_H
#define RUNTIME_SIMRUNTIME_H

#include "../Runtime.h"

namespace mlir {
namespace aegis {


class SimRuntime : public Runtime {
public:
    SimRuntime(const std::string &mlirFileName)    {
        assert(!mlirFileName.empty());
        simMlirFileName = mlirFileName;
    }
    virtual ~SimRuntime() {}

public:
    llvm::Expected<bool> open(const std::string &sharedLibPath) override;
    llvm::Expected<bool> loadCryptoResources(const std::string &cryptCtxFileName,
                                             const std::string &pubKeyFileName, 
                                             const std::string &multKeyFileName, 
                                             const std::string &rotKeyFileName) override;
    llvm::Expected<bool> resolveSymbol(const std::string &funcName) override;
    llvm::Expected<std::vector<Value>> call(const std::vector<Value> &input) override;
    llvm::Expected<std::vector<Value>> simulate(const std::vector<Value> &input) override;

private:
    llvm::Expected<bool> replaceMlirTokenWith(const std::vector<std::string> &replacements);

private:
    std::string simMlirFileName;
};


} // namespace aegis
} // namespace mlir

#endif // RUNTIME_SIMRUNTIME_H