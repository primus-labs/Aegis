// RUN: test_cpp_to_bin.sh  %s | FileCheck %s

#include "openfhe.h"
#include <vector>
#include <iostream>

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
#define AddPlain(c, p) AddPlainImpl((c), (p))
#define SubPlain(c, p) SubPlainImpl((c), (p))
#define Mul(a, b) clientCC->EvalMult((a), (b))
#define MulPlain(c, p) MulPlainImpl((c), (p))
#define Rotate(c, idx) clientCC->EvalRotate((c), (idx))
#define Bootstrap(a) clientCC->EvalBootstrap(a)
#define MakePlain(a)  double(a)
#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}
#define LoadPlainWithIndex(v, idx) v[idx]
#define LoadPlainWithoutIndex(v) v[0]
#define ConstantArray_to_PlainVector(ary) std::vector<double>(ary, ary + std::size(ary))
#define Cast_Plain_To_Index(clr) size_t(clr)
#define Cast_Plain_To_Cipher(clr) clientCC->Encrypt(clientPubKey, clientCC->MakeCKKSPackedPlaintext(std::vector<double>(clr)))
#define Cast_Stub(a) a
#define Vload(arg, idx) arg[idx]                
#define Vstore(val, arg, idx) arg[idx] = val   



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


static CryptoContext<DCRTPoly> clientCC;
static PublicKey<DCRTPoly> clientPubKey;
void initCryptContext() {
    int ccSizes = CryptoContextFactory<DCRTPoly>::GetContextCount();
    if (ccSizes > 0) {
        std::vector<uint32_t> levelBudget = {3, 1};
        unsigned mulDepth = 18;
        if (0) {
            SecretKeyDist secretKeyDist = UNIFORM_TERNARY;
            mulDepth += FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist);
        }
        CCParams<CryptoContextCKKSRNS> parameters;
        parameters.SetMultiplicativeDepth(mulDepth);
        parameters.SetFirstModSize(60);
        parameters.SetScalingModSize(50);
        parameters.SetBatchSize(1);
        clientCC = GenCryptoContext(parameters);
        if (!Serial::DeserializeFromFile(pubKeyFileName, clientPubKey, SerType::BINARY)) {
            std::cerr << "Cannot read serialized data from: " << pubKeyFileName << std::endl;
            std::exit(1);
        }
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
template <typename T1, typename T2>
auto Add(T1&& a, T2&& b) -> decltype(auto) {
    return clientCC->EvalAdd(std::forward<T1>(a), std::forward<T2>(b));
}
template <typename T1, typename T2, typename... Ts>
auto Add(T1&& a, T2&& b, Ts&&... rest) {
    return Add(clientCC->EvalAdd(std::forward<T1>(a), std::forward<T2>(b)), std::forward<Ts>(rest)...);
}
template <typename T1, typename T2>
auto Sub(T1&& a, T2&& b) -> decltype(auto) {
    return clientCC->EvalSub(std::forward<T1>(a), std::forward<T2>(b));
}

template <typename T1, typename T2, typename... Ts>
auto Sub(T1&& a, T2&& b, Ts&&... rest) {
    return Sub(clientCC->EvalSub(std::forward<T1>(a), std::forward<T2>(b)), std::forward<Ts>(rest)...);
}


// RLWECipher Alloc(size_t batch_size) {
//     if (size > 1)
//         size = 1;
//     std::vector<double> constVec(size, 0.0);
//     Plaintext plaintext = clientCC->MakeCKKSPackedPlaintext(constVec);
//     return clientCC->Encrypt(clientPubKey, plaintext);
// }
// inline RLWECipher Alloc() {
//     if (1 >= 16)
//         return Alloc(16);
//     else
//         return Alloc(1);
// }

#define BATCH_SIZE 2
RLWECipher Alloc(size_t size) {
    std::vector<double> constVec(BATCH_SIZE, 0.0);
    Plaintext plaintext = clientCC->MakeCKKSPackedPlaintext(constVec);
    return clientCC->Encrypt(clientPubKey, plaintext);
}
inline RLWECipher Alloc() {
    return Alloc(BATCH_SIZE);
}
std::vector<RLWECipher> AllocArray(size_t row, size_t batch_size) {
    return std::vector<RLWECipher>(row, Alloc(batch_size));
}

std::vector<RLWECipher> main_graph(std::vector<RLWECipher> v1, std::vector<RLWECipher> v2) {
  PlainVector v3 = MakeMultPlain(0,1);
  PlainVector v4 = MakeMultPlain(1,0);
  Plain v5 = MakePlain(2);
  Plain v6 = MakePlain(1);
  Plain v7 = MakePlain(0);
  size_t v8 = Cast_Plain_To_Index(v7);
  size_t v9 = Cast_Plain_To_Index(v6);
  size_t v10 = Cast_Plain_To_Index(v5);
//   std::vector<RLWECipher> v11 = Alloc(16);
  std::vector<RLWECipher> v11 = AllocArray(3, 16);
  RLWECipher v12 = Vload(v1, v8);
  RLWECipher v13 = Vload(v2, v8);
  RLWECipher v14 = Add(v12, v13);
  RLWECipher v15 = MulPlain(v14, v4);
  RLWECipher v16 = Vload(v11, v8);
  RLWECipher v17 = MulPlain(v16, v3);
  RLWECipher v18 = Add(v17, v15);
  Vstore(v18, v11, v8);
  RLWECipher v19 = Vload(v1, v8);
  RLWECipher v20 = Vload(v2, v8);
  RLWECipher v21 = Add(v19, v20);
  RLWECipher v22 = MulPlain(v21, v3);
  RLWECipher v23 = Vload(v11, v8);
  RLWECipher v24 = MulPlain(v23, v4);
  RLWECipher v25 = Add(v24, v22);
  Vstore(v25, v11, v8);
  RLWECipher v26 = Vload(v1, v9);
  RLWECipher v27 = Vload(v2, v9);
  RLWECipher v28 = Add(v26, v27);
  RLWECipher v29 = MulPlain(v28, v4);
  RLWECipher v30 = Vload(v11, v9);
  RLWECipher v31 = MulPlain(v30, v3);
  RLWECipher v32 = Add(v31, v29);
  Vstore(v32, v11, v9);
  RLWECipher v33 = Vload(v1, v9);
  RLWECipher v34 = Vload(v2, v9);
  RLWECipher v35 = Add(v33, v34);
  RLWECipher v36 = MulPlain(v35, v3);
  RLWECipher v37 = Vload(v11, v9);
  RLWECipher v38 = MulPlain(v37, v4);
  RLWECipher v39 = Add(v38, v36);
  Vstore(v39, v11, v9);
  RLWECipher v40 = Vload(v1, v10);
  RLWECipher v41 = Vload(v2, v10);
  RLWECipher v42 = Add(v40, v41);
  RLWECipher v43 = MulPlain(v42, v4);
  RLWECipher v44 = Vload(v11, v10);
  RLWECipher v45 = MulPlain(v44, v3);
  RLWECipher v46 = Add(v45, v43);
  Vstore(v46, v11, v10);
  RLWECipher v47 = Vload(v1, v10);
  RLWECipher v48 = Vload(v2, v10);
  RLWECipher v49 = Add(v47, v48);
  RLWECipher v50 = MulPlain(v49, v3);
  RLWECipher v51 = Vload(v11, v10);
  RLWECipher v52 = MulPlain(v51, v4);
  RLWECipher v53 = Add(v52, v50);
  Vstore(v53, v11, v10);
  return v11;
}


// extern "C" 
// std::vector<uint8_t> aegis_mlir_main_graph(const std::vector<uint8_t> &buf1, const std::vector<uint8_t> &buf2) {
//     initCryptContext();

//     Ciphertext<DCRTPoly> v1;
//     std::stringstream ss1;
//     ss1.write(reinterpret_cast<const char *>(buf1.data()), buf1.size());
//     Serial::Deserialize(v1, ss1, SerType::BINARY);

//     Ciphertext<DCRTPoly> v2;
//     std::stringstream ss2;
//     ss2.write(reinterpret_cast<const char *>(buf2.data()), buf2.size());
//     Serial::Deserialize(v2, ss2, SerType::BINARY);

//     RLWECipher retV = main_graph(v1, v2);
//     std::stringstream retss;
//     Serial::Serialize(retV, retss, SerType::BINARY);
//     std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
//     return retBuf;
// }

extern "C" 
std::vector<std::vector<uint8_t>> aegis_mlir_main_graph(const std::vector<std::vector<uint8_t>> &buf1, 
                                                        const std::vector<std::vector<uint8_t>> &buf2) {
    initCryptContext();

    std::vector<RLWECipher> v1;
    for (auto i = 0; i < buf1.size(); i++) {
        RLWECipher tmp1;
        std::stringstream ss1;
        ss1.write(reinterpret_cast<const char *>(buf1[i].data()), buf1[i].size());
        Serial::Deserialize(tmp1, ss1, SerType::BINARY);
        v1.push_back(tmp1);
    }

    std::vector<RLWECipher> v2;
    for (auto j = 0; j < buf2.size(); j++) {
        RLWECipher tmp2;
        std::stringstream ss2;
        ss2.write(reinterpret_cast<const char *>(buf2[j].data()), buf2[j].size());
        Serial::Deserialize(tmp2, ss2, SerType::BINARY);
        v2.push_back(tmp2);
    }

    std::vector<RLWECipher> retV = main_graph(v1, v2);

    std::vector<std::vector<uint8_t>> resBuf;
    for (auto k = 0; k < retV.size(); k++) {
        std::stringstream retss;
        Serial::Serialize(retV[k], retss, SerType::BINARY);
        std::vector<uint8_t> tmpBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
        resBuf.push_back(tmpBuf);
    }
    return resBuf;
}


//CHECK: Pass