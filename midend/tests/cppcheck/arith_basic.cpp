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
using Plain = double;
using PlainVector = std::vector<double>;
using PlainMatrix = std::vector<PlainVector>;
using MutableCiphertextT = Ciphertext<DCRTPoly>;
using CCParamsT = CCParams<CryptoContextCKKSRNS>;
using CryptoContextT = CryptoContext<DCRTPoly>;
using EvalKeyT = EvalKey<DCRTPoly>;
using PrivateKeyT = PrivateKey<DCRTPoly>;
using PublicKeyT = PublicKey<DCRTPoly>;


//---------  begin added  -------------
// #define Add(a, b) cryptoCtx->EvalAdd((a), (b))
// #define Add(...) Add_IMPL(__VA_ARGS__)
// #define Add_IMPL(a, b, ...) cryptoCtx->EvalAdd(Add_IMPL(a, b), ##__VA_ARGS__)
// #define Add_IMPL(a, b) cryptoCtx->EvalAdd((a), (b))
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

inline RLWECipher AddPlainImpl(RLWECipher a, Plain b) {
            return cryptoCtx->EvalAdd(a, b);
        }
        inline RLWECipher AddPlainImpl(RLWECipher a, PlainVector b) {
            return cryptoCtx->EvalAdd(a, cryptoCtx->MakeCKKSPackedPlaintext(b));
        }
        inline RLWECipher SubPlainImpl(RLWECipher a, Plain b) {
            return cryptoCtx->EvalSub(a, b);
        }
        inline RLWECipher SubPlainImpl(RLWECipher a, PlainVector b) {
            return cryptoCtx->EvalSub(a, cryptoCtx->MakeCKKSPackedPlaintext(b));
        }
        inline RLWECipher MulPlainImpl(RLWECipher a, Plain b) {
            return cryptoCtx->EvalMult(a, b);
        }
        inline RLWECipher MulPlainImpl(RLWECipher a, PlainVector b) {
            return cryptoCtx->EvalMult(a, cryptoCtx->MakeCKKSPackedPlaintext(b));
        }
        template <typename T1, typename T2>
        auto Add(T1&& a, T2&& b) -> decltype(auto) {
            return cryptoCtx->EvalAdd(std::forward<T1>(a), std::forward<T2>(b));
        }
        template <typename T1, typename T2, typename... Ts>
        auto Add(T1&& a, T2&& b, Ts&&... rest) {
            return Add(cryptoCtx->EvalAdd(std::forward<T1>(a), std::forward<T2>(b)), std::forward<Ts>(rest)...);
        }
        template <typename T1, typename T2>
        auto Sub(T1&& a, T2&& b) -> decltype(auto) {
            return cryptoCtx->EvalSub(std::forward<T1>(a), std::forward<T2>(b));
        }

        template <typename T1, typename T2, typename... Ts>
        auto Sub(T1&& a, T2&& b, Ts&&... rest) {
            return Sub(cryptoCtx->EvalSub(std::forward<T1>(a), std::forward<T2>(b)), std::forward<Ts>(rest)...);
        }

RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
  init_cryptcontext();
  RLWECipher v3 = Add(v1, v2);
  RLWECipher v4 = Sub(v1, v2);
  RLWECipher v5 = Add(v1, v2, v3, v4, v1, v2, v3, v3);
  RLWECipher v6 = Sub(v1, v2, v4, v3, v2, v1, v4, v3);
  RLWECipher v7 = Mul(v5, v6);
  return v7;
}


//CHECK: Pass

