// RUN: aegiscompiler --collect-metadata -arith-to-secret --func-to-secret --canonicalize --cse < %s | FileCheck %s


module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  memref.global @constant_0 : memref<f32> = dense<2.000000e+00>

  func.func @main_graph(%arg0: memref<8xf32> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                        %arg1: memref<8xf32> {onnx.name = "input_y", onnx.type = "encrypted"}) -> memref<8xf32> {
    memref.copy %arg0, %arg1 : memref<8xf32> to memref<8xf32>
    return %arg1 : memref<8xf32>
  }
}



//CHECK: func.func @main_graph(%arg0: !secret.secret_vector<8 x f32> {onnx.name = "input_x", onnx.type = "encrypted"}
