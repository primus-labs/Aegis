// RUN: test_single_bin_file.sh  %s | FileCheck %s

#include <vector>
#include "openfhe.h"
using namespace std;
using namespace lbcrypto;
using CiphertextT = Ciphertext<DCRTPoly>;
using RLWECipher = Ciphertext<DCRTPoly>;
using LWECipher = Ciphertext<DCRTPoly>;
using PlaintextT = Plaintext;
// using Plain = Plaintext;
// using PlainVector = Plaintext;  
// using PlainMatrix = Plaintext;
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
#define Add(a, b) cryptoCtx->EvalAdd((a), (b))
//#define AddPlain(c, p) cryptoCtx->EvalAdd((c), (p))
#define AddPlain(c, p) AddPlainImpl((c), (p))
#define Sub(a, b) cryptoCtx->EvalSub((a), (b))
//#define SubPlain(c, p) cryptoCtx->EvalSub((c), (p))
#define SubPlain(c, p) SubPlainImpl((c), (p))
#define Mul(a, b) cryptoCtx->EvalMult((a), (b))
//#define MulPlain(c, p) cryptoCtx->EvalMult((c), (p))
#define MulPlain(c, p) MulPlainImpl((c), (p)) 
#define Rotate(c, idx) cryptoCtx->EvalRotate((c), (idx))
//#define MakePlain(...)  cryptoCtx->MakeCKKSPackedPlaintext(std::vector<double>{__VA_ARGS__})
#define MakePlain(a) double(a) 
#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__} 
//#define Cast_Plain_To_Index(pt) pt->GetRealPackedValue()[0]
#define Cast_Plain_To_Index(clr) size_t(clr)
#define Native_Load(v, idx) v[idx]
CryptoContext<DCRTPoly> cryptoCtx;
void init_cryptcontext() {
   CCParams<CryptoContextCKKSRNS> parameters;
   parameters.SetMultiplicativeDepth(8);
   parameters.SetFirstModSize(60);
   parameters.SetScalingModSize(50);
   parameters.SetBatchSize(2048);
   cryptoCtx = GenCryptoContext(parameters);
   cryptoCtx->Enable(PKE);
   cryptoCtx->Enable(KEYSWITCH);
   cryptoCtx->Enable(LEVELEDSHE);
}
inline RLWECipher MulPlainImpl(RLWECipher a, Plain b) {
    return cryptoCtx->EvalMult(a, b);
}
inline RLWECipher MulPlainImpl(RLWECipher a, PlainVector b) {
    return cryptoCtx->EvalMult(a, cryptoCtx->MakeCKKSPackedPlaintext(b));
}
inline RLWECipher SubPlainImpl(RLWECipher a, Plain b) {
    return cryptoCtx->EvalSub(a, b);
}
inline RLWECipher SubPlainImpl(RLWECipher a, PlainVector b) {
    return cryptoCtx->EvalSub(a, cryptoCtx->MakeCKKSPackedPlaintext(b));
}
inline RLWECipher AddPlainImpl(RLWECipher a, Plain b) {
    return cryptoCtx->EvalAdd(a, b);
}
inline RLWECipher AddPlainImpl(RLWECipher a, PlainVector b) {
    return cryptoCtx->EvalAdd(a, cryptoCtx->MakeCKKSPackedPlaintext(b));
}
extern "C"
RLWECipher MVP(RLWECipher v1, RLWECipher v2) {
  PlainVector v3 = MakeMultPlain(1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
  PlainVector v4 = MakeMultPlain(0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);
  init_cryptcontext();
  RLWECipher v5 = Mul(v1, v2);
  RLWECipher v6 = Rotate(v5, 15);
  RLWECipher v7 = Add(v5, v6);
  RLWECipher v8 = MulPlain(v7, v3);
  RLWECipher v9 = MulPlain(v2, v4);
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
  RLWECipher v19 = MulPlain(v18, v3);
  RLWECipher v20 = Add(v9, v19);
  Copy(v20, v2);
  return v2;
}

//CHECK: Pass
