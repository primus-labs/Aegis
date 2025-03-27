// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module {
  emitc.include <"vector">
  emitc.include "openfhe.h"
  emitc.verbatim "using namespace std;"
  emitc.verbatim "using namespace lbcrypto;"
  emitc.verbatim "using CiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using PlaintextT = Plaintext;"
  emitc.verbatim "using MutableCiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using CCParamsT = CCParams<CryptoContextCKKSRNS>;"
  emitc.verbatim "using CryptoContextT = CryptoContext<DCRTPoly>;"
  emitc.verbatim "using EvalKeyT = EvalKey<DCRTPoly>;"
  emitc.verbatim "using PrivateKeyT = PrivateKey<DCRTPoly>;"
  emitc.verbatim "using PublicKeyT = PublicKey<DCRTPoly>;"
  func.func @main_graph(%arg0: !emitc.opaque<"RLWECipher"> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"RLWECipher"> {
    %0 = emitc.call_opaque "Sub"(%arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %1 = emitc.call_opaque "Add"(%arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %2 = emitc.call_opaque "Mul"(%0, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    return %2 : !emitc.opaque<"RLWECipher">
  }
}



// CHECK-NOT: emitc.verbatim
// CHECK-NOT: emitc.call_opaque
// CHECK: #include <vector>
// CHECK: #include "openfhe.h"
// CHECK: using namespace std;
// CHECK: using namespace lbcrypto;
// CHECK: using CiphertextT = Ciphertext<DCRTPoly>;
// CHECK: using PlaintextT = Plaintext;
// CHECK: using MutableCiphertextT = Ciphertext<DCRTPoly>;
// CHECK: using CCParamsT = CCParams<CryptoContextCKKSRNS>;
// CHECK: using CryptoContextT = CryptoContext<DCRTPoly>;
// CHECK: using EvalKeyT = EvalKey<DCRTPoly>;
// CHECK: using PrivateKeyT = PrivateKey<DCRTPoly>;
// CHECK: using PublicKeyT = PublicKey<DCRTPoly>;
// CHECK: RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
// CHECK:   RLWECipher v3 = Sub(v1, v2);
// CHECK:   RLWECipher v4 = Add(v1, v2);
// CHECK:   RLWECipher v5 = Mul(v3, v4);
// CHECK:   return v5;
// CHECK: }