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
#define Alloc() RLWECipher()    //added
#define Cast_Plain_To_Index(clr) size_t(clr)
#define LoadPlainWithIndex(v, idx) v[idx]
#define LoadPlainWithoutIndex(v) v[0]
#define Cast_Stub(a) a        
#define ConstantArray_to_PlainVector(ary) std::vector<double>(ary, ary + std::size(ary)) 

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


extern float constant_0[1] = {2.000000000e+00f};
RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
  PlainVector v3 = MakeMultPlain(1,1,1,1,1,1,1,0);
  PlainVector v4 = MakeMultPlain(0,0,0,0,0,0,0,1);
  PlainVector v5 = MakeMultPlain(1,1,1,1,1,1,0,1);
  PlainVector v6 = MakeMultPlain(0,0,0,0,0,0,1,0);
  PlainVector v7 = MakeMultPlain(1,1,1,1,1,0,1,1);
  PlainVector v8 = MakeMultPlain(0,0,0,0,0,1,0,0);
  PlainVector v9 = MakeMultPlain(1,1,1,1,0,1,1,1);
  PlainVector v10 = MakeMultPlain(0,0,0,0,1,0,0,0);
  PlainVector v11 = MakeMultPlain(1,1,1,0,1,1,1,1);
  PlainVector v12 = MakeMultPlain(0,0,0,1,0,0,0,0);
  PlainVector v13 = MakeMultPlain(1,1,0,1,1,1,1,1);
  PlainVector v14 = MakeMultPlain(0,0,1,0,0,0,0,0);
  PlainVector v15 = MakeMultPlain(1,0,1,1,1,1,1,1);
  PlainVector v16 = MakeMultPlain(0,1,0,0,0,0,0,0);
  PlainVector v17 = MakeMultPlain(0,1,1,1,1,1,1,1);
  PlainVector v18 = MakeMultPlain(1,0,0,0,0,0,0,0);
  PlainVector v19 = ConstantArray_to_PlainVector(constant_0);
  Plain v20 = LoadPlainWithoutIndex(v19);
  RLWECipher v21 = MulPlain(v2, v20);
  RLWECipher v22 = Alloc();
  RLWECipher v23 = Add(v1, v21);
  RLWECipher v24 = MulPlain(v23, v18);
  RLWECipher v25 = MulPlain(v22, v17);
  RLWECipher v26 = Add(v25, v24);
  Copy(v26, v22);
  RLWECipher v27 = Rotate(v21, -1);
  RLWECipher v28 = Add(v1, v27);
  RLWECipher v29 = MulPlain(v28, v16);
  RLWECipher v30 = MulPlain(v22, v15);
  RLWECipher v31 = Add(v30, v29);
  Copy(v31, v22);
  RLWECipher v32 = Rotate(v21, -2);
  RLWECipher v33 = Add(v1, v32);
  RLWECipher v34 = MulPlain(v33, v14);
  RLWECipher v35 = MulPlain(v22, v13);
  RLWECipher v36 = Add(v35, v34);
  Copy(v36, v22);
  RLWECipher v37 = Rotate(v21, -3);
  RLWECipher v38 = Add(v1, v37);
  RLWECipher v39 = MulPlain(v38, v12);
  RLWECipher v40 = MulPlain(v22, v11);
  RLWECipher v41 = Add(v40, v39);
  Copy(v41, v22);
  RLWECipher v42 = Rotate(v21, -4);
  RLWECipher v43 = Add(v1, v42);
  RLWECipher v44 = MulPlain(v43, v10);
  RLWECipher v45 = MulPlain(v22, v9);
  RLWECipher v46 = Add(v45, v44);
  Copy(v46, v22);
  RLWECipher v47 = Rotate(v21, -5);
  RLWECipher v48 = Add(v1, v47);
  RLWECipher v49 = MulPlain(v48, v8);
  RLWECipher v50 = MulPlain(v22, v7);
  RLWECipher v51 = Add(v50, v49);
  Copy(v51, v22);
  RLWECipher v52 = Rotate(v21, -6);
  RLWECipher v53 = Add(v1, v52);
  RLWECipher v54 = MulPlain(v53, v6);
  RLWECipher v55 = MulPlain(v22, v5);
  RLWECipher v56 = Add(v55, v54);
  Copy(v56, v22);
  RLWECipher v57 = Rotate(v21, -7);
  RLWECipher v58 = Add(v1, v57);
  RLWECipher v59 = MulPlain(v58, v4);
  RLWECipher v60 = MulPlain(v22, v3);
  RLWECipher v61 = Add(v60, v59);
  Copy(v61, v22);
  return v22;
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

    RLWECipher retV = main_graph(v1, v2);
    std::stringstream retss;
    Serial::Serialize(retV, retss, SerType::BINARY);
    std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
    return retBuf;
}


//CHECK: Pass