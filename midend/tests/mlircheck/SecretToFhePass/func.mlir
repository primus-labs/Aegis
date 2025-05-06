// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse < %s | FileCheck %s
module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {

  func.func @test_func(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
    %5 = arith.addf %arg0, %arg1 :  f32
    return %5 : f32
  }
}


// CHECK-LABEL: func.func @test_func
// CHECK: (%[[ARG0:[a-zA-Z0-9_]+]]: !fhe.lwecipher<f32> {onnx.dims = [1], onnx.name = "input_x", onnx.type = "encrypted"}, %[[ARG1:[a-zA-Z0-9_]+]]: !fhe.lwecipher<f32> {onnx.dims = [1], onnx.name = "input_y", onnx.type = "encrypted"}) -> (!fhe.lwecipher<f32> {onnx.dims = [1]})




