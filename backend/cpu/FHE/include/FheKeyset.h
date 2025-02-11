#ifndef CPU_FHE_OPENFHE_FHEKEYSET_H
#define CPU_FHE_OPENFHE_FHEKEYSET_H

#include <memory>
#include <vector>
#include <iostream>
#include "FheKey.h"
#include "openfhe.h"

using namespace lbcrypto;

namespace aegislang {

class FheKeyset {
private:
    std::shared_ptr<FHEPrivateKey> priKey; 
    std::shared_ptr<FHEPublicKey> pubKey; 
    std::shared_ptr<FHERelinKey> relinKey;
    std::shared_ptr<FHERotateKey> rotKey; 
    std::shared_ptr<FHEBootstrapKey> bsKey; 

public:
    FheKeyset(std::shared_ptr<FHEPrivateKey> PriKey, std::shared_ptr<FHEPublicKey> PubKey, std::shared_ptr<FHERelinKey> RelinKey,
            std::shared_ptr<FHERotateKey> RotKey, std::shared_ptr<FHEBootstrapKey> BsKey)
        : priKey(std::move(PriKey)), pubKey(std::move(PubKey)), relinKey(std::move(RelinKey)), 
          rotKey(std::move(RotKey)), bsKey(std::move(BsKey)) {}
    
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

}

#endif

