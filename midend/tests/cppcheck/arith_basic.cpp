// #include <vector>
// #include "openfhe.h"
// using namespace std;
// using namespace lbcrypto;
// using CiphertextT = ConstCiphertext<DCRTPoly>;
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
using CiphertextT = Ciphertext<DCRTPoly>;       //Modified
using RLWECipher = Ciphertext<DCRTPoly>;        //Added
using PlaintextT = Plaintext;
using MutableCiphertextT = Ciphertext<DCRTPoly>;
using CCParamsT = CCParams<CryptoContextCKKSRNS>;
using CryptoContextT = CryptoContext<DCRTPoly>;
using EvalKeyT = EvalKey<DCRTPoly>;
using PrivateKeyT = PrivateKey<DCRTPoly>;
using PublicKeyT = PublicKey<DCRTPoly>;

//---------  begin added  -------------
#define Add(a, b) cryptoContext->EvalAdd((a), (b))
#define Mul(a, b) cryptoContext->EvalMult((a), (b))
CryptoContext<DCRTPoly> cryptoContext;
void init_cryptcontext() {
    CCParams<CryptoContextBGVRNS> parameters;
    parameters.SetMultiplicativeDepth(2);
    
    cryptoContext = GenCryptoContext(parameters);
    cryptoContext->Enable(PKE);
    cryptoContext->Enable(KEYSWITCH);
    cryptoContext->Enable(LEVELEDSHE);
}
//---------   end added   -------------


RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
  RLWECipher v4 = Add(v1, v2);
  RLWECipher v5 = Mul(v2, v4);
  return v5;
}


