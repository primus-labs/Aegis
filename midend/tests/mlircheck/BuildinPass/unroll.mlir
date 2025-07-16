// RUN: aegiscompiler --unroll-loop-and-memory-opt --canonicalize --cse < %s | FileCheck %s

// #map = affine_map<(d0) -> (d0)>
// module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "addmul_model"} {
//   memref.global @constant_0 : memref<f32> = dense<2.000000e+00>

//   func.func @main_graph(%arg0: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_x"}, %arg1: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_y"}) -> (memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "output"}) attributes {llvm.emit_c_interface} {
//     %c0 = arith.constant 0 : index
//     //%0 = "krnl.global"() {name = "constant_0", shape = [], value = dense<2.000000e+00> : tensor<f32>} : () -> memref<f32>
//     %0 = memref.get_global @constant_0 : memref<f32>
//     %dim = memref.dim %arg1, %c0 : memref<?xf32>
//     %alloc = memref.alloc(%dim) {alignment = 16 : i64} : memref<?xf32>
//     affine.for %arg2 = 0 to #map(%dim) {
//       %1 = affine.load %arg1[%arg2] : memref<?xf32>
//       %2 = affine.load %0[] : memref<f32>
//       %3 = arith.mulf %1, %2 : f32
//       %4 = affine.load %arg0[%arg2] : memref<?xf32>
//       %5 = arith.addf %4, %3 : f32
//       affine.store %5, %alloc[%arg2] : memref<?xf32>
//     }
//     return %alloc : memref<?xf32>
//   }
// }

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  memref.global @constant_0 : memref<f32> = dense<2.000000e+00>

  func.func @main_graph(%arg0: memref<8xf32> {onnx.name = "input_x"}, %arg1: memref<1xf32> {onnx.name = "input_y"}) -> (memref<8xf32> {onnx.name = "output"}) attributes {llvm.emit_c_interface} {
    %c0 = arith.constant 0 : index
    %0 = memref.get_global @constant_0 : memref<f32>
    %alloc = memref.alloc() {alignment = 16 : i64} : memref<1xf32>
    %1 = affine.load %arg1[%c0] : memref<1xf32>
    %2 = affine.load %0[] : memref<f32>
    %3 = arith.mulf %1, %2 : f32
    affine.store %3, %alloc[%c0] : memref<1xf32>
    %alloc_0 = memref.alloc() {alignment = 16 : i64} : memref<8xf32>
    affine.for %arg2 = 0 to 8 {
      %4 = affine.load %arg0[%arg2] : memref<8xf32>
      %5 = affine.load %alloc[%c0] : memref<1xf32>
      %6 = arith.addf %4, %5 : f32
      affine.store %6, %alloc_0[%arg2] : memref<8xf32>
    }
    return %alloc_0 : memref<8xf32>
  }
}


// CHECK-NOT: affine.for
