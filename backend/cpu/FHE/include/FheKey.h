#ifndef CPU_FHE_OPENFHE_FHEKEY_H
#define CPU_FHE_OPENFHE_FHEKEY_H

#include "Key.h"
#include "openfhe.h"

using namespace lbcrypto;

namespace aegiscpu {
namespace openfhe {

class FHEPrivateKey : public Key {
  private:
    PrivateKey<DCRTPoly> privateKey;

  public:
    FHEPrivateKey() {}
    FHEPrivateKey(const PrivateKey<DCRTPoly> &key) : privateKey(key) {}

    std::string serialize() const override {
        std::stringstream ss;
        Serial::Serialize(privateKey, ss, SerType::BINARY);
        return ss.str();
    }

    void deserialize(const std::string &serializedData) override {
        std::stringstream ss(serializedData);
        Serial::Deserialize(privateKey, ss, SerType::BINARY);
    }

    PrivateKey<DCRTPoly> getKey() const { return privateKey; }
};

class FHEPublicKey : public Key {
  private:
    PublicKey<DCRTPoly> publicKey;

  public:
    FHEPublicKey() {}
    FHEPublicKey(const PublicKey<DCRTPoly> &key) : publicKey(key) {}

    std::string serialize() const override {
        std::stringstream ss;
        Serial::Serialize(publicKey, ss, SerType::BINARY);
        return ss.str();
    }

    void deserialize(const std::string &serializedData) override {
        std::stringstream ss(serializedData);
        Serial::Deserialize(publicKey, ss, SerType::BINARY);
    }

    PublicKey<DCRTPoly> getKey() const { return publicKey; }
};

} // namespace openfhe
} // namespace aegiscpu

#endif
