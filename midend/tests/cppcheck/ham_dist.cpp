// RUN: test_cpp_to_bin.sh  %s | FileCheck %s

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
#define Cast_Plain_To_Cipher(clr) clientCC->Encrypt(clientPubKey, clientCC->MakeCKKSPackedPlaintext(std::vector<double>(clr)))
#define LoadPlainWithIndex(v, idx) v[idx]
#define LoadPlainWithoutIndex(v) v[0]
#define Cast_Stub(a) a        
#define ConstantArray_to_PlainVector(ary) std::vector<double>(ary, ary + std::size(ary)) 
#define Cmp_ue(a, b) a       //refine and add
#define Select(cond, a, b) a //refine and add

static CryptoContext<DCRTPoly> clientCC;
static PublicKey<DCRTPoly> clientPubKey;
static std::string ccFileName, pubKeyFileName, mulKeyFileName, rotKeyFileName;

extern "C"
bool loadCryptoResources(const std::string &ccLoc,  const std::string &pubKeyLoc, 
                         const std::string &multKeyLoc, const std::string &rotKeyLoc) {
    ccFileName = ccLoc;
    pubKeyFileName = pubKeyLoc;
    mulKeyFileName = multKeyLoc;
    rotKeyFileName = rotKeyLoc;
    return true;
}


void initCryptContext() {
    int ccSizes = CryptoContextFactory<DCRTPoly>::GetContextCount();
    std::cout << "before call GenCryptoContext, crypto context obj counts:" << ccSizes << std::endl;
    if (ccSizes > 0) {
        // use exist crypto context obj
        std::vector<uint32_t> levelBudget = {3, 1};
        unsigned mulDepth = 8;
        if (1) {
            SecretKeyDist secretKeyDist = UNIFORM_TERNARY;
            mulDepth += FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist);
        }
        CCParams<CryptoContextCKKSRNS> parameters;
        parameters.SetMultiplicativeDepth(mulDepth);
        parameters.SetFirstModSize(60);
        parameters.SetScalingModSize(50);
        parameters.SetBatchSize(1);
        clientCC = GenCryptoContext(parameters);
        ccSizes = CryptoContextFactory<DCRTPoly>::GetContextCount();
        std::cout << "after call GenCryptoContext, crypto context obj counts:" << ccSizes << std::endl;
    } else {
        clientCC->ClearEvalMultKeys();
        clientCC->ClearEvalAutomorphismKeys();
        lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
        if (!Serial::DeserializeFromFile(ccFileName, clientCC, SerType::BINARY)) {
            std::cerr << "Cannot read serialized data from: " << ccFileName << std::endl;
            std::exit(1);
        }

        if (!Serial::DeserializeFromFile(pubKeyFileName, clientPubKey, SerType::BINARY)) {
            std::cerr << "Cannot read serialized data from: " << pubKeyFileName << std::endl;
            std::exit(1);
        }

        std::ifstream multKeyIStream(mulKeyFileName, std::ios::in | std::ios::binary);
        if (!multKeyIStream.is_open()) {
            std::cerr << "Cannot read serialization from " << mulKeyFileName << std::endl;
            std::exit(1);
        }
        if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {
            std::cerr << "Could not deserialize eval mult key file" << std::endl;
            std::exit(1);
        }

        if (!rotKeyFileName.empty()) {
            std::ifstream rotKeyIStream(rotKeyFileName, std::ios::in | std::ios::binary);
            if (!rotKeyIStream.is_open()) {
                std::cerr << "Cannot read serialization from " << rotKeyFileName << std::endl;
                std::exit(1);
            }
            if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {
                std::cerr << "Could not deserialize eval rot key file" << std::endl;
                std::exit(1);
            }
        }
    }
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


RLWECipher hamming_distance(RLWECipher v1, RLWECipher v2) {
  PlainVector v3 = MakeMultPlain(0,0,0,1);
  PlainVector v4 = MakeMultPlain(0,0,1,0);
  PlainVector v5 = MakeMultPlain(0,1,0,0);
  Plain v6 = MakePlain(1.000000);
  Plain v7 = MakePlain(0.000000);
  PlainVector v8 = MakeMultPlain(1,0,0,0);
  RLWECipher v9 = MulPlain(v1, v8);
  RLWECipher v10 = MulPlain(v2, v8);
  RLWECipher v11 = Cmp_ue(v9, v10);
  RLWECipher v12 = Cast_Plain_To_Cipher(v6);
  RLWECipher v13 = Cast_Plain_To_Cipher(v7);
  RLWECipher v14 = Select(v11, v12, v13);
  RLWECipher v15 = MulPlain(v1, v5);
  RLWECipher v16 = MulPlain(v2, v5);
  RLWECipher v17 = Cmp_ue(v15, v16);
  RLWECipher v18 = Select(v17, v12, v13);
  RLWECipher v19 = Add(v14, v18);
  RLWECipher v20 = MulPlain(v1, v4);
  RLWECipher v21 = MulPlain(v2, v4);
  RLWECipher v22 = Cmp_ue(v20, v21);
  RLWECipher v23 = Select(v22, v12, v13);
  RLWECipher v24 = Add(v19, v23);
  RLWECipher v25 = MulPlain(v1, v3);
  RLWECipher v26 = MulPlain(v2, v3);
  RLWECipher v27 = Cmp_ue(v25, v26);
  RLWECipher v28 = Select(v27, v12, v13);
  RLWECipher v29 = Add(v24, v28);
  return v29;
}



extern "C" 
std::vector<uint8_t> aegis_mlir_MVP(const std::vector<uint8_t> &buf1, const std::vector<uint8_t> &buf2) {
    initCryptContext();

    Ciphertext<DCRTPoly> v1;
    std::stringstream ss1;
    ss1.write(reinterpret_cast<const char *>(buf1.data()), buf1.size());
    Serial::Deserialize(v1, ss1, SerType::BINARY);

    Ciphertext<DCRTPoly> v2;
    std::stringstream ss2;
    ss2.write(reinterpret_cast<const char *>(buf2.data()), buf2.size());
    Serial::Deserialize(v2, ss2, SerType::BINARY);

    RLWECipher retV = hamming_distance(v1, v2);
    std::stringstream retss;
    Serial::Serialize(retV, retss, SerType::BINARY);
    std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
    return retBuf;
}


//CHECK: Pass