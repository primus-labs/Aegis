// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --insert-emitc-preamble --cse < %s | FileCheck %s

module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.addf %arg0, %arg1 :  f32
        %6 = arith.mulf %arg0, %5 :  f32
        return %6 : f32
    }
}


// CHECK: emitc.include <"vector">
// CHECK: emitc.include "openfhe.h"
// CHECK: emitc.verbatim "using namespace std;"
// CHECK: emitc.verbatim "using namespace lbcrypto;"
// CHECK: emitc.verbatim "using CiphertextT = ConstCiphertext<DCRTPoly>;"
// CHECK: emitc.verbatim "using PlaintextT = Plaintext;"
// CHECK: emitc.verbatim "using MutableCiphertextT = Ciphertext<DCRTPoly>;"
// CHECK: emitc.verbatim "using CCParamsT = CCParams<CryptoContextCKKSRNS>;"
// CHECK: emitc.verbatim "using CryptoContextT = CryptoContext<DCRTPoly>;"
// CHECK: emitc.verbatim "using EvalKeyT = EvalKey<DCRTPoly>;"
// CHECK: emitc.verbatim "using PrivateKeyT = PrivateKey<DCRTPoly>;"
// CHECK: emitc.verbatim "using PublicKeyT = PublicKey<DCRTPoly>;"
// CHECK: emitc.call_opaque "Add"
// CHECK: emitc.call_opaque "Mul"

