// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module {
  emitc.include <"vector">
  emitc.include "openfhe.h"
  emitc.verbatim "using namespace std;"
  emitc.verbatim "using namespace lbcrypto;"
  emitc.verbatim "using CiphertextT = ConstCiphertext<DCRTPoly>;"
  emitc.verbatim "using PlaintextT = Plaintext;"
  emitc.verbatim "using MutableCiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using CCParamsT = CCParams<CryptoContextCKKSRNS>;"
  emitc.verbatim "using CryptoContextT = CryptoContext<DCRTPoly>;"
  emitc.verbatim "using EvalKeyT = EvalKey<DCRTPoly>;"
  emitc.verbatim "using PrivateKeyT = PrivateKey<DCRTPoly>;"
  emitc.verbatim "using PublicKeyT = PublicKey<DCRTPoly>;"
  func.func @main_graph(%arg0: !emitc.opaque<"LWECipher"> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"Plain"> {onnx.name = "input_y", onnx.type = "clear"}) -> !emitc.opaque<"LWECipher"> {
    %0 = emitc.call_opaque "AddPlain"(%arg0, %arg1) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"LWECipher">
    %1 = emitc.call_opaque "Mul"(%arg0, %0) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"LWECipher">) -> !emitc.opaque<"LWECipher">
    return %1 : !emitc.opaque<"LWECipher">
  }
}


// CHECK: #include <vector>
// CHECK: #include "openfhe.h"
// CHECK: using namespace std;
// CHECK: using namespace lbcrypto;
// CHECK: using CiphertextT = ConstCiphertext<DCRTPoly>;
// CHECK: using PlaintextT = Plaintext;
// CHECK: using MutableCiphertextT = Ciphertext<DCRTPoly>;
// CHECK: using CCParamsT = CCParams<CryptoContextCKKSRNS>;
// CHECK: using CryptoContextT = CryptoContext<DCRTPoly>;
// CHECK: using EvalKeyT = EvalKey<DCRTPoly>;
// CHECK: using PrivateKeyT = PrivateKey<DCRTPoly>;
// CHECK: using PublicKeyT = PublicKey<DCRTPoly>;

