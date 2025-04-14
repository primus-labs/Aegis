#include "Operate.h"
#include "CryptoContextMgr.h"
#include "FheKeyset.h"

namespace aegiscpu {

std::vector<uint8_t> encrypt(const std::vector<double> &data) {
    CryptoContext<DCRTPoly> cc = CryptoContextMgr::getInstance().getCryptoContext();

    Plaintext ptValue = cc->MakeCKKSPackedPlaintext(data);

    PublicKey<DCRTPoly> pk = FheKeyset::getInstance().getPubKey()->getKey();
    Ciphertext<DCRTPoly> ctValue = cc->Encrypt(pk, ptValue);

    std::stringstream ss;
    Serial::Serialize(ctValue, ss, SerType::BINARY);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>());

    return buffer;
}

std::vector<double> decrypt(const std::vector<uint8_t> &data, size_t plaintextSize) {
    CryptoContext<DCRTPoly> cc = CryptoContextMgr::getInstance().getCryptoContext();

    // Deserialize output byte array into a ciphertext object.
    std::stringstream ss;
    ss.write(reinterpret_cast<const char *>(data.data()), data.size());
    Ciphertext<DCRTPoly> deserCiphertext;
    Serial::Deserialize(deserCiphertext, ss, SerType::BINARY);

    // get private key
    PrivateKey<DCRTPoly> priKey = FheKeyset::getInstance().getPriKey()->getKey();

    // decrypt
    Plaintext ptValue;
    cc->Decrypt(priKey, deserCiphertext, &ptValue);
    ptValue->SetLength(plaintextSize); // Need know plaintext size for decrypting.

    return ptValue->GetRealPackedValue();
}

void serializeCiphertext(const Ciphertext<DCRTPoly>& ciphertext, std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    Serial::Serialize(ciphertext, ss, SerType::BINARY);
    std::string serialized = ss.str();
    bytes.assign(serialized.begin(), serialized.end());
}

Ciphertext<DCRTPoly> deserializeCiphertext(const std::vector<uint8_t> &bytes) {
    Ciphertext<DCRTPoly> newC;
    std::string serialized(bytes.begin(), bytes.end());
    std::stringstream ss(serialized);
    Serial::Deserialize<Ciphertext<DCRTPoly>>(newC, ss, SerType::BINARY);
    return newC;
}
    
} // namespace aegiscpu