#ifndef CPU_FHE_OPENFHE_FHEKEY_H
#define CPU_FHE_OPENFHE_FHEKEY_H


#include "Key.h"
#include "openfhe.h"

using namespace lbcrypto;

namespace aegislang {

class FHEPrivateKey : public Key {
private:
    PrivateKey<DCRTPoly> privateKey;

public:
    FHEPrivateKey(const PrivateKey<DCRTPoly>& key) : privateKey(key) {}

    std::string serialize() const override {
        std::stringstream ss;
        Serial::Serialize(privateKey, ss, SerType::BINARY);
        return ss.str();
    }

    void deserialize(const std::string& serializedData) override {
        std::stringstream ss(serializedData);
        Serial::Deserialize(privateKey, ss, SerType::BINARY);
    }

    PrivateKey<DCRTPoly> getKey() const {
        return privateKey;
    }
};


class FHEPublicKey : public Key {
private:
    PublicKey<DCRTPoly> publicKey;

public:
    FHEPublicKey(const PublicKey<DCRTPoly>& key) : publicKey(key) {}

    std::string serialize() const override {
        std::stringstream ss;
        Serial::Serialize(publicKey, ss, SerType::BINARY);
        return ss.str();
    }

    void deserialize(const std::string& serializedData) override {
        std::stringstream ss(serializedData);
        Serial::Deserialize(publicKey, ss, SerType::BINARY);
    }

    PublicKey<DCRTPoly> getKey() const {
        return publicKey;
    }
};


class FHERelinKey : public Key {
private:
    std::string relinKeySer;

public:
    FHERelinKey(const std::string& key) : relinKeySer(key) {}

    std::string serialize() const override {
        return relinKeySer;
    }

    void deserialize(const std::string& serializedData) override {
        relinKeySer = serializedData;
    }
};


class FHERotateKey : public Key {
private:
    std::string rotKeySer;

public:
    FHERotateKey(const std::string& key) : rotKeySer(key) {}

    std::string serialize() const override {
        return rotKeySer;
    }

    void deserialize(const std::string& serializedData) override {
        rotKeySer = serializedData;
    }
};


class FHEBootstrapKey : public Key {
private:
    std::string bsKeySer;

public:
    FHEBootstrapKey(const std::string& key) : bsKeySer(key) {}

    std::string serialize() const override {
        return bsKeySer;
    }

    void deserialize(const std::string& serializedData) override {
        bsKeySer = serializedData;
    }
};

}

#endif
