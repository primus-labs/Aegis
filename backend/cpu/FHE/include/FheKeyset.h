#ifndef CPU_FHE_OPENFHE_FHEKEYSET_H
#define CPU_FHE_OPENFHE_FHEKEYSET_H

#include "FheKey.h"
#include "openfhe.h"
#include <iostream>
#include <memory>
#include <vector>

using namespace lbcrypto;

namespace aegiscpu {
namespace openfhe {

class FheKeyset {
  private:
    std::shared_ptr<FHEPrivateKey> priKey;
    std::shared_ptr<FHEPublicKey> pubKey;

  private:
    FheKeyset(std::shared_ptr<FHEPrivateKey> PriKey, std::shared_ptr<FHEPublicKey> PubKey)
        : priKey(std::move(PriKey)), pubKey(std::move(PubKey)) {}

    // Delete the copy constructor and the assignment operator.
    FheKeyset(const FheKeyset &) = delete;
    FheKeyset &operator=(const FheKeyset &) = delete;

  public:
    // static method to get the singleton instance.
    static FheKeyset &getInstance() {
        static FheKeyset instance(std::make_shared<FHEPrivateKey>(), std::make_shared<FHEPublicKey>());
        return instance;
    }

    // initialize the singleton instance.
    static void initialize(std::shared_ptr<FHEPrivateKey> PriKey, std::shared_ptr<FHEPublicKey> PubKey) {
        static std::once_flag flag;
        std::call_once(flag, [&]() {
            auto &instance = getInstance();
            instance.priKey = std::move(PriKey);
            instance.pubKey = std::move(PubKey);
        });
    }

  public:
    std::shared_ptr<FHEPrivateKey> getPriKey() const { return priKey; }
    std::shared_ptr<FHEPublicKey> getPubKey() const { return pubKey; }

    // TODO: if need to implement in binding level??
    std::string dumps(bool contain_sk = true);
    void loads(const std::string &keys);
};

} // namespace openfhe
} // namespace aegiscpu

#endif
