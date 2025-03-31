// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module {
  emitc.include <"vector">
  emitc.include "openfhe.h"
  emitc.verbatim "using namespace std;"
  emitc.verbatim "using namespace lbcrypto;"
  emitc.verbatim "using CiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using RLWECipher = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using LWECipher = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using PlaintextT = Plaintext;"
  emitc.verbatim "using MutableCiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using CCParamsT = CCParams<CryptoContextCKKSRNS>;"
  emitc.verbatim "using CryptoContextT = CryptoContext<DCRTPoly>;"
  emitc.verbatim "using EvalKeyT = EvalKey<DCRTPoly>;"
  emitc.verbatim "using PrivateKeyT = PrivateKey<DCRTPoly>;"
  emitc.verbatim "using PublicKeyT = PublicKey<DCRTPoly>;"
  emitc.verbatim "#define Add(a, b) cryptoCtx->EvalAdd((a), (b))"
  emitc.verbatim "#define Mul(a, b) cryptoCtx->EvalMult((a), (b))"
  emitc.verbatim "CryptoContext<DCRTPoly> cryptoCtx;"
  emitc.verbatim "void init_cryptcontext() {"
  emitc.verbatim "   CCParams<CryptoContextCKKSRNS> parameters;"
  emitc.verbatim "   //TODO"
  emitc.verbatim "   cryptoCtx = GenCryptoContext(parameters);"
  emitc.verbatim "   cryptoCtx->Enable(PKE);"
  emitc.verbatim "   cryptoCtx->Enable(KEYSWITCH);"
  emitc.verbatim "   cryptoCtx->Enable(LEVELEDSHE);"
  emitc.verbatim "}"
  func.func @main_graph(%arg0: !emitc.opaque<"LWECipher"> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"LWECipher"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"LWECipher"> {
    emitc.verbatim "init_cryptcontext();"
    %0 = emitc.call_opaque "Add"(%arg0, %arg1) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"LWECipher">) -> !emitc.opaque<"LWECipher">
    %1 = emitc.call_opaque "Mul"(%arg0, %0) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"LWECipher">) -> !emitc.opaque<"LWECipher">
    return %1 : !emitc.opaque<"LWECipher">
  }
}


// CHECK: LWECipher main_graph(LWECipher v1, LWECipher v2) {
// CHECK:   init_cryptcontext();
// CHECK:   LWECipher v3 = Add(v1, v2);
// CHECK:   LWECipher v4 = Mul(v1, v3);
// CHECK:   return v4;
// CHECK: }