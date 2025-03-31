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
#define Mul(a, b) cryptoCtx->EvalMult((a), (b))
#define MulPlain(c, p) cryptoCtx->EvalMult((c), (p))
#define Rotate(c, idx) cryptoCtx->EvalRotate((c), (idx))
#define MakePlain(...)  cryptoCtx->MakeCKKSPackedPlaintext(std::vector<double>{__VA_ARGS__})
#define Cast_Plain_To_Index(pt) pt->GetRealPackedValue()[0]
#define Native_Load(v, idx) v[idx]

CryptoContext<DCRTPoly> cryptoCtx;
void init_cryptcontext() {
   CCParams<CryptoContextBGVRNS> parameters;
   //TODO
   cryptoCtx = GenCryptoContext(parameters);
   cryptoCtx->Enable(PKE);
   cryptoCtx->Enable(KEYSWITCH);
   cryptoCtx->Enable(LEVELEDSHE);
}

RLWECipher MVP(std::vector<Plain> v1, RLWECipher v2) {
  init_cryptcontext();
  Plain v3 = MakePlain(0.000000);
  Plain v4 = MakePlain(1.000000);
  Plain v5 = MakePlain(4.000000);
  Plain v6 = MakePlain(5.000000);
  size_t v7 = Cast_Plain_To_Index(v6);
  size_t v8 = Cast_Plain_To_Index(v5);
  size_t v9 = Cast_Plain_To_Index(v4);
  size_t v10 = Cast_Plain_To_Index(v3);
  Plain v11 = Native_Load(v1, v10);
  RLWECipher v12 = MulPlain(v2, v11);
  Plain v13 = Native_Load(v1, v9);
  RLWECipher v14 = MulPlain(v2, v13);
  RLWECipher v15 = Rotate(v14, 3);
  RLWECipher v16 = Add(v12, v15);
  Plain v19 = Native_Load(v1, v8);
  RLWECipher v20 = MulPlain(v2, v19);
  Plain v21 = Native_Load(v1, v7);
  RLWECipher v22 = MulPlain(v2, v21);
  RLWECipher v23 = Rotate(v20, 1);
  RLWECipher v24 = Add(v23, v22);
  return v2;
}