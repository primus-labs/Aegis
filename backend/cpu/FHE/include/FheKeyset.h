#ifndef CPU_FHE_OPENFHE_FHEKEYSET_H
#define CPU_FHE_OPENFHE_FHEKEYSET_H

#include <memory>
#include <vector>
#include <iostream>
#include "FheKey.h"
#include "openfhe.h"

using namespace lbcrypto;

namespace aegiscpu {

class FheKeyset {
private:
    std::shared_ptr<FHEPrivateKey> priKey; 
    std::shared_ptr<FHEPublicKey> pubKey; 
    std::shared_ptr<FHERelinKey> relinKey;
    std::shared_ptr<FHERotateKey> rotKey; 
    std::shared_ptr<FHEBootstrapKey> bsKey; 

private:
    FheKeyset(std::shared_ptr<FHEPrivateKey> PriKey, std::shared_ptr<FHEPublicKey> PubKey, std::shared_ptr<FHERelinKey> RelinKey,
            std::shared_ptr<FHERotateKey> RotKey, std::shared_ptr<FHEBootstrapKey> BsKey)
        : priKey(std::move(PriKey)), pubKey(std::move(PubKey)), relinKey(std::move(RelinKey)), 
          rotKey(std::move(RotKey)), bsKey(std::move(BsKey)) {}

    // Delete the copy constructor and the assignment operator.
    FheKeyset(const FheKeyset&) = delete;
    FheKeyset& operator=(const FheKeyset&) = delete;

public:
    // static method to get the singleton instance.
    static FheKeyset& getInstance() {
        static FheKeyset instance(nullptr, nullptr, nullptr, nullptr, nullptr);
        return instance;
    }

    // initialize the singleton instance.
    static void initialize(std::shared_ptr<FHEPrivateKey> PriKey, std::shared_ptr<FHEPublicKey> PubKey,
                           std::shared_ptr<FHERelinKey> RelinKey, std::shared_ptr<FHERotateKey> RotKey,
                           std::shared_ptr<FHEBootstrapKey> BsKey) {
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            auto& instance = getInstance();
            instance.priKey = std::move(PriKey);
            instance.pubKey = std::move(PubKey);
            instance.relinKey = std::move(RelinKey);
            instance.rotKey = std::move(RotKey);
            instance.bsKey = std::move(BsKey);
        });
    }

public:
    std::shared_ptr<FHEPrivateKey> getPriKey() const {
        return priKey;
    }

    std::shared_ptr<FHEPublicKey> getPubKey() const {
        return pubKey;
    }

    std::shared_ptr<FHERelinKey> getRelinKey() const {
        return relinKey;
    }

    std::shared_ptr<FHERotateKey> getRotateKey() const {
        return rotKey;
    }

    std::shared_ptr<FHEBootstrapKey> getBootstrapKey() const {
        return bsKey;
    }
};

} // namespace aegiscpu

#endif

