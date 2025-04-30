// RUN: test_single_bin_file.sh  %s | FileCheck %s

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
#define Bootstrap(a) clientCC->EvalBootstrap(a)
#define MakePlain(a)  double(a)
#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}
#define Cast_Plain_To_Index(clr) size_t(clr)
#define Native_Load(v, idx) v[idx]

CryptoContext<DCRTPoly> clientCC;
PublicKey<DCRTPoly> clientPubKey;
extern "C"
bool init_cryptcontext(const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {
    std::vector<uint32_t> levelBudget = {3, 1};
    unsigned mulDepth = 8;
    if (0) {
        SecretKeyDist secretKeyDist = UNIFORM_TERNARY;
        mulDepth += FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist);
    }
    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(mulDepth);
    parameters.SetFirstModSize(60);
    parameters.SetScalingModSize(50);
    parameters.SetBatchSize(16);
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
RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
  RLWECipher v3 = Mul(v1, v2);
  RLWECipher v4 = Mul(v3, v1);
  RLWECipher v5 = Mul(v3, v4);
  RLWECipher v6 = Mul(v4, v5);
  RLWECipher v7 = Mul(v5, v6);
  RLWECipher v8 = Mul(v6, v7);
  RLWECipher v9 = Mul(v7, v8);
  RLWECipher v10 = Bootstrap(v9);
  RLWECipher v11 = Mul(v8, v10);
  RLWECipher v12 = Bootstrap(v11);
  RLWECipher v13 = Mul(v10, v12);
  return v13;
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

    RLWECipher retV = main_graph(v1, v2);
    std::stringstream retss;
    Serial::Serialize(retV, retss, SerType::BINARY);
    std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
    return retBuf;
}


//CHECK: Pass