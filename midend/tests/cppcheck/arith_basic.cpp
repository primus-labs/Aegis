// RUN: test_cpp_to_bin.sh  %s | FileCheck %s

// #include <vector>
// #include "openfhe.h"
// using namespace std;
// using namespace lbcrypto;
// using CiphertextT = Ciphertext<DCRTPoly>;
// using PlaintextT = Plaintext;
// using MutableCiphertextT = Ciphertext<DCRTPoly>;
// using CCParamsT = CCParams<CryptoContextCKKSRNS>;
// using CryptoContextT = CryptoContext<DCRTPoly>;
// using EvalKeyT = EvalKey<DCRTPoly>;
// using PrivateKeyT = PrivateKey<DCRTPoly>;
// using PublicKeyT = PublicKey<DCRTPoly>;
// RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
//   RLWECipher v3 = Sub(v1, v2);
//   RLWECipher v4 = Add(v1, v2);
//   RLWECipher v5 = Mul(v3, v4);
//   return v5;
// }



#include <vector>
#include "openfhe.h"
using namespace std;
using namespace lbcrypto;
using CiphertextT = Ciphertext<DCRTPoly>;       
using RLWECipher = Ciphertext<DCRTPoly>;       
using LWECipher = Ciphertext<DCRTPoly>;        
using PlaintextT = Plaintext;
using MutableCiphertextT = Ciphertext<DCRTPoly>;
using CCParamsT = CCParams<CryptoContextCKKSRNS>;
using CryptoContextT = CryptoContext<DCRTPoly>;
using EvalKeyT = EvalKey<DCRTPoly>;
using PrivateKeyT = PrivateKey<DCRTPoly>;
using PublicKeyT = PublicKey<DCRTPoly>;

//---------  begin added  -------------
#define Add(a, b) cryptoCtx->EvalAdd((a), (b))
#define Mul(a, b) cryptoCtx->EvalMult((a), (b))
CryptoContext<DCRTPoly> cryptoCtx;
void init_cryptcontext() {
    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(2);
    
    cryptoCtx = GenCryptoContext(parameters);
    cryptoCtx->Enable(PKE);
    cryptoCtx->Enable(KEYSWITCH);
    cryptoCtx->Enable(LEVELEDSHE);
}
//---------   end added   -------------


RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
  init_cryptcontext();
  RLWECipher v4 = Add(v1, v2);
  RLWECipher v5 = Mul(v2, v4);
  return v5;
}


//CHECK: Pass

