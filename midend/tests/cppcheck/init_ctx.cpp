// RUN: test_cpp_to_bin.sh  %s | FileCheck %s

#include <vector>
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
#define LoadPlainWithIndex(v, idx) v[idx]

CryptoContext<DCRTPoly> clientCC;
PublicKey<DCRTPoly> clientPubKey;
extern "C"
bool init_cryptcontext(const std::string &ccLoc, const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {
    clientCC->ClearEvalMultKeys();
    clientCC->ClearEvalAutomorphismKeys();
    lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
    if (!Serial::DeserializeFromFile(ccLoc, clientCC, SerType::BINARY)) {
        std::cerr << "Cannot read serialized data from: " << ccLoc << std::endl;
        return false;
    }
    if (!Serial::DeserializeFromFile(pubKeyLoc, clientPubKey, SerType::BINARY)) {
        std::cerr << "I cannot read serialized data from: " << pubKeyLoc << std::endl;
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
extern "C"
RLWECipher MVP(PlainVector v1, RLWECipher v2) {
  PlainVector v3 = MakeMultPlain(0,1,1,1);
  PlainVector v4 = MakeMultPlain(1,0,0,0);
  Plain v5 = MakePlain(0);
  Plain v6 = MakePlain(1);
  Plain v7 = MakePlain(4);
  Plain v8 = MakePlain(5);
  size_t v9 = Cast_Plain_To_Index(v8);
  size_t v10 = Cast_Plain_To_Index(v7);
  size_t v11 = Cast_Plain_To_Index(v6);
  size_t v12 = Cast_Plain_To_Index(v5);
  Plain v13 = LoadPlainWithIndex(v1, v12);
  RLWECipher v14 = MulPlain(v2, v13);
  Plain v15 = LoadPlainWithIndex(v1, v11);
  RLWECipher v16 = MulPlain(v2, v15);
  RLWECipher v17 = Rotate(v16, 3);
  RLWECipher v18 = Add(v14, v17);
  RLWECipher v19 = MulPlain(v18, v4);
  RLWECipher v20 = MulPlain(v2, v3);
  RLWECipher v21 = Add(v20, v19);
  Copy(v21, v2);
  Plain v22 = LoadPlainWithIndex(v1, v10);
  RLWECipher v23 = MulPlain(v2, v22);
  Plain v24 = LoadPlainWithIndex(v1, v9);
  RLWECipher v25 = MulPlain(v2, v24);
  RLWECipher v26 = Rotate(v23, 1);
  RLWECipher v27 = Add(v26, v25);
  RLWECipher v28 = Rotate(v27, -1);
  RLWECipher v29 = MulPlain(v28, v4);
  RLWECipher v30 = Add(v20, v29);
  Copy(v30, v2);
  return v2;
}

//CHECK: Pass