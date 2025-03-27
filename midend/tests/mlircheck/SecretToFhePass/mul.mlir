// RUN: aegiscompiler -collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  func.func @main_mul(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                      %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
    %5 = arith.mulf %arg0, %arg1 :  f32
    return %5 : f32

    // CHECK-NOT: arith.mulf
    // CHECK-NOT: secret.mul
    // CHECK: fhe.lwemul
  }


  func.func @main_mul_encrypt_vs_clear(%arg0: f32 {onnx.name = "input_x2", onnx.type = "encrypted"}, 
                                       %arg1: f32 {onnx.name = "input_y2", onnx.type = "clear"}) -> f32 {
    %5 = arith.mulf %arg0, %arg1 :  f32
    return %5 : f32

    // CHECK-NOT: arith.mulf
    // CHECK-NOT: secret.mulplain
    // CHECK: fhe.lwemul_plain
  }


  func.func @main_mul_clear_vs_encrypt(%arg0: f32 {onnx.name = "input_x3", onnx.type = "clear"}, 
                                       %arg1: f32 {onnx.name = "input_y3", onnx.type = "encrypted"}) -> f32 {
    %5 = arith.mulf %arg0, %arg1 :  f32
    return %5 : f32

    // CHECK-NOT: arith.mulf
    // CHECK-NOT: secret.mulplain
    // CHECK: fhe.lwemul_plain
  }
}



