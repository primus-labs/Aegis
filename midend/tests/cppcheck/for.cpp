#include <vector>
#include <iostream>
#include "openfhe.h"
using namespace std;
using namespace lbcrypto;
using CiphertextT = Ciphertext<DCRTPoly>;
using RLWECipher = Ciphertext<DCRTPoly>;
using LWECipher = Ciphertext<DCRTPoly>;
using PlaintextT = Plaintext;
using Plain = double;
using PlainVector = std::vector<double>;
using PlainMatrix = std::vector<PlainVector>;
using MutableCiphertextT = Ciphertext<DCRTPoly>;
using CCParamsT = CCParams<CryptoContextCKKSRNS>;
using CryptoContextT = CryptoContext<DCRTPoly>;
using EvalKeyT = EvalKey<DCRTPoly>;
using PrivateKeyT = PrivateKey<DCRTPoly>;
using PublicKeyT = PublicKey<DCRTPoly>;
#define Copy(src, dest) dest = src
#define Add(a, b) clientCC->EvalAdd((a), (b))
#define AddPlain(c, p) AddPlainImpl((c), (p))
#define Sub(a, b) clientCC->EvalSub((a), (b))
#define SubPlain(c, p) SubPlainImpl((c), (p))
#define Mul(a, b) clientCC->EvalMult((a), (b))
#define MulPlain(c, p) MulPlainImpl((c), (p))
#define Rotate(c, idx) clientCC->EvalRotate((c), (idx))
#define MakePlain(a)  double(a)
#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}
#define Cast_Plain_To_Index(clr) size_t(clr)
#define Native_Load(v, idx) v[idx]

CryptoContext<DCRTPoly> clientCC;
PublicKey<DCRTPoly> clientPubKey;
extern "C"
bool init_cryptcontext(const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {
    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(8);
    parameters.SetFirstModSize(60);
    parameters.SetScalingModSize(50);
    parameters.SetBatchSize(2048);
    clientCC = GenCryptoContext(parameters);
    clientCC->ClearEvalMultKeys();
    clientCC->ClearEvalAutomorphismKeys();
    if (!Serial::DeserializeFromFile(pubKeyLoc, clientPubKey, SerType::BINARY)) {
        std::cerr << "Cannot read serialized data from: " << pubKeyLoc << std::endl;
        return false;
    }
    std::ifstream multKeyIStream(multKeyLoc, std::ios::in | std::ios::binary);
    if (!multKeyIStream.is_open()) {
        std::cerr << "Cannot read serialization from " << multKeyLoc << std::endl;
        return false;
    }
    if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {
        std::cerr << "Could not deserialize eval mult key file" << std::endl;
        return false;
    }
    if (!rotKeyLoc.empty()) {
        std::ifstream rotKeyIStream(rotKeyLoc, std::ios::in | std::ios::binary);
        if (!rotKeyIStream.is_open()) {
            std::cerr << "Cannot read serialization from " << rotKeyLoc << std::endl;
            return false;
        }
        if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {
            std::cerr << "Could not deserialize eval rot key file" << std::endl;
            return false;
        }
    }
    return true;
}

inline RLWECipher AddPlainImpl(RLWECipher a, Plain b) {
    return clientCC->EvalAdd(a, b);
}
inline RLWECipher AddPlainImpl(RLWECipher a, PlainVector b) {
    return clientCC->EvalAdd(a, clientCC->MakeCKKSPackedPlaintext(b));
}
inline RLWECipher SubPlainImpl(RLWECipher a, Plain b) {
    return clientCC->EvalSub(a, b);
}
inline RLWECipher SubPlainImpl(RLWECipher a, PlainVector b) {
    return clientCC->EvalSub(a, clientCC->MakeCKKSPackedPlaintext(b));
}
inline RLWECipher MulPlainImpl(RLWECipher a, Plain b) {
    return clientCC->EvalMult(a, b);
}
inline RLWECipher MulPlainImpl(RLWECipher a, PlainVector b) {
    return clientCC->EvalMult(a, clientCC->MakeCKKSPackedPlaintext(b));
}
RLWECipher MVP(RLWECipher v1, RLWECipher v2) {
  PlainVector v3 = MakeMultPlain(0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);
  PlainVector v4 = MakeMultPlain(1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
  RLWECipher v5 = Mul(v1, v2);
  RLWECipher v6 = Rotate(v5, 15);
  RLWECipher v7 = Add(v5, v6);
  RLWECipher v8 = MulPlain(v7, v4);
  RLWECipher v9 = MulPlain(v2, v3);
  RLWECipher v10 = Add(v9, v8);
  Copy(v10, v2);
  RLWECipher v11 = Rotate(v2, 4);
  RLWECipher v12 = Mul(v1, v11);
  RLWECipher v13 = Rotate(v2, 4);
  RLWECipher v14 = Mul(v1, v13);
  RLWECipher v15 = Rotate(v12, 13);
  RLWECipher v16 = Rotate(v14, 12);
  RLWECipher v17 = Add(v15, v16);
  RLWECipher v18 = Rotate(v17, -1);
  RLWECipher v19 = MulPlain(v18, v4);
  RLWECipher v20 = Add(v9, v19);
  Copy(v20, v2); 
  return v2;
}


extern "C" 
std::vector<uint8_t> aegis_mlir_MVP(const std::vector<uint8_t> &buf1, const std::vector<uint8_t> &buf2) {

    Ciphertext<DCRTPoly> v1;
    std::stringstream ss1;
    ss1.write(reinterpret_cast<const char *>(buf1.data()), buf1.size());
    Serial::Deserialize(v1, ss1, SerType::BINARY);

    Ciphertext<DCRTPoly> v2;
    std::stringstream ss2;
    ss2.write(reinterpret_cast<const char *>(buf2.data()), buf2.size());
    Serial::Deserialize(v2, ss2, SerType::BINARY);


    RLWECipher retV = MVP(v1, v2);

    std::stringstream retss;
    Serial::Serialize(retV, retss, SerType::BINARY);
    std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
    return retBuf;
}