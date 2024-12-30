#ifndef CPU_FHE_OPENFHE_FHEKEYSET_H
#define CPU_FHE_OPENFHE_FHEKEYSET_H

#include "FheKey.h"
#include <memory>


class FheKeyset {
private:
    std::unique_ptr<FHESecretKey> pri_key;
    std::unique_ptr<FHEPublicKey> pub_key;
    std::unique_ptr<FHEComputationKey> eva_key;

public:
    FheKeyset():pri_key(nullptr), pub_key(nullptr), eva_key(nullptr) {}
    
    void addKey(KEY_TYPE keyType, std::unique_ptr<Key> key);
    std::unique_ptr<Key> getKey(KEY_TYPE keyType) const;
};


#endif

