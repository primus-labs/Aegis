// RUN: aegiscompiler --collect-metadata  --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse  < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  memref.global @constant_0 : memref<f32> = dense<2.000000e+00>
  func.func @main_graph(%arg0: !secret.secret<f32> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                        %arg1: !secret.secret<f32> {onnx.name = "input_y", onnx.type = "encrypted"}) 
                        -> !secret.secret<f32> attributes {llvm.emit_c_interface} {
    %4 = "secret.mul"(%arg1, %arg0) : (!secret.secret<f32>, !secret.secret<f32>) -> !secret.secret<f32>
    %5 = "secret.reveal"(%4) : (!secret.secret<f32>) -> f32
    return %4 : !secret.secret<f32>

    // CHECK-NOT: secret.mul
    // CHECK-NOT: secret.reveal
    // CHECK: fhe.lwemul
    // CHECK: fhe.reveal
  }
}