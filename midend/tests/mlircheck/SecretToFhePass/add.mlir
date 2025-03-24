// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe < %s | FileCheck %s
module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  // memref.global @constant_0 : memref<f32> = dense<2.000000e+00>
  // func.func @main_graph(%arg0: memref<8xf32> {onnx.name = "input_x"}, %arg1: memref<1xf32> {onnx.name = "input_y"}) -> (memref<8xf32> {onnx.name = "output"}) attributes {llvm.emit_c_interface} {
  //   %c0 = arith.constant 0 : index
  //   %0 = memref.get_global @constant_0 : memref<f32>
  //   %alloc = memref.alloc() {alignment = 16 : i64} : memref<1xf32>
  //   %1 = affine.load %arg1[%c0] : memref<1xf32>
  //   %2 = affine.load %0[] : memref<f32>
  //   %3 = arith.mulf %1, %2 : f32
  //   affine.store %3, %alloc[%c0] : memref<1xf32>
  //   %alloc_0 = memref.alloc() {alignment = 16 : i64} : memref<8xf32>
  //   affine.for %arg2 = 0 to 8 {
  //     %4 = affine.load %arg0[%arg2] : memref<8xf32>
  //     %5 = affine.load %alloc[%c0] : memref<1xf32>
  //     %6 = arith.addf %4, %5 : f32
  //     affine.store %6, %alloc_0[%arg2] : memref<8xf32>
  //   }
  //   return %alloc_0 : memref<8xf32>
  // }


  func.func @main_add(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                        %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
    %5 = arith.addf %arg0, %arg1 :  f32
    return %5 : f32

    // CHECK-NOT: arith.addf
    // CHECK-NOT: secret.add
    // CHECK: fhe.lweadd
  }


  func.func @main_add_encrypt_vs_clear(%arg0: f32 {onnx.name = "input_x2", onnx.type = "encrypted"}, 
                                       %arg1: f32 {onnx.name = "input_y2", onnx.type = "clear"}) -> f32 {
    %5 = arith.addf %arg0, %arg1 :  f32
    return %5 : f32

    // CHECK-NOT: arith.addf
    // CHECK-NOT: secret.addplain
    // CHECK: fhe.lweaddplain
  }


  func.func @main_add_clear_vs_encrypt(%arg0: f32 {onnx.name = "input_x3", onnx.type = "clear"}, 
                                       %arg1: f32 {onnx.name = "input_y3", onnx.type = "encrypted"}) -> f32 {
    %5 = arith.addf %arg0, %arg1 :  f32
    return %5 : f32

    // CHECK-NOT: arith.addf
    // CHECK-NOT: secret.addplain
    // CHECK: fhe.lweaddplain
  }
}



