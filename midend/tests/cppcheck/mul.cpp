// RUN: test_single_bin_file.sh  %s | FileCheck %s

#include <vector>
#include "openfhe.h"
using namespace std;
using namespace lbcrypto;
using CiphertextT = Ciphertext<DCRTPoly>;
using RLWECipher = Ciphertext<DCRTPoly>;
using LWECipher = Ciphertext<DCRTPoly>;
using PlaintextT = Plaintext;
using Plain = Plaintext;
using MutableCiphertextT = Ciphertext<DCRTPoly>;
using CCParamsT = CCParams<CryptoContextCKKSRNS>;
using CryptoContextT = CryptoContext<DCRTPoly>;
using EvalKeyT = EvalKey<DCRTPoly>;
using PrivateKeyT = PrivateKey<DCRTPoly>;
using PublicKeyT = PublicKey<DCRTPoly>;
#define Add(a, b) cryptoCtx->EvalAdd((a), (b))
#define AddPlain(c, p) cryptoCtx->EvalAdd((c), (p))
#define Sub(a, b) cryptoCtx->EvalSub((a), (b))
#define SubPlain(c, p) cryptoCtx->EvalSub((c), (p))
#define Mul(a, b) cryptoCtx->EvalMult((a), (b))
#define MulPlain(c, p) cryptoCtx->EvalMult((c), (p))
#define Rotate(c, idx) cryptoCtx->EvalRotate((c), (idx))
#define MakePlain(...)  cryptoCtx->MakeCKKSPackedPlaintext(std::vector<double>{__VA_ARGS__})
#define Cast_Plain_To_Index(pt) pt->GetRealPackedValue()[0]
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

extern "C"
RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
  init_cryptcontext();
  RLWECipher v3 = Mul(v1, v2);
  return v3;
}


//CHECK: Pass