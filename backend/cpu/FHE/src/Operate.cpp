#include "Operate.h"
#include "CryptoContextMgr.h"
#include "FheKeyset.h"
#include <cassert>

namespace aegiscpu {
namespace openfhe {

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

std::vector<std::vector<double>> decryptBatch(const std::vector<uint8_t> &data, size_t n, size_t plaintextSize) {
    CryptoContext<DCRTPoly> cc = CryptoContextMgr::getInstance().getCryptoContext();

    std::stringstream ss;
    ss.write(reinterpret_cast<const char *>(data.data()), data.size());

    // get private key
    PrivateKey<DCRTPoly> priKey = FheKeyset::getInstance().getPriKey()->getKey();

    // Loop through each serialized ciphertext and decrypt
    std::vector<Plaintext> decryptedTexts;
    while (ss && ss.peek() != EOF) {
        try {
            // Deserialize single ciphertext
            Ciphertext<DCRTPoly> ct;
            Serial::Deserialize(ct, ss, SerType::BINARY);
            
            // Decrypt ciphertext
            Plaintext pt;
            cc->Decrypt(priKey, ct, &pt);
            decryptedTexts.push_back(pt);
        }
        catch (const std::exception& e) {
            std::cerr << "decryptBatch error: " << e.what() << std::endl;
            break;
        }
    }
    assert(decryptedTexts.size() == n);

    // get plaintext values
    std::vector<std::vector<double>> result;
    for (auto i = 0; i < decryptedTexts.size(); i++) {
        Plaintext ptValue = decryptedTexts[i];
        ptValue->SetLength(plaintextSize); 
        auto realVal = ptValue->GetRealPackedValue();
        result.emplace_back(realVal);
    }

    return result;
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

} // namespace openfhe
} // namespace aegiscpu