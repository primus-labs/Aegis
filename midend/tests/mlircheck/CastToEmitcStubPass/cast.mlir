// RUN: aegiscompiler --collect-metadata  --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub  < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  memref.global @constant_0 : memref<f32> = dense<2.000000e+00>
  func.func @main_graph() -> !secret.secret<f32> attributes {llvm.emit_c_interface} {
    %c0 = arith.constant 1.0 : f32
    %1 = "secret.cast"(%c0) : (f32) -> !secret.secret<f32>
    return %1 : !secret.secret<f32>


    // CHECK-NOT: secret.cast
    // CHECK-NOT: fhe.secret
    // CHECK: emitc.call_opaque "Cast_Plain_To_Cipher"
  }

}