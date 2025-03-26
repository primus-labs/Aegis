// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse < %s | FileCheck %s
module {
  func.func @MVP(%arg0: memref<16xf64> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                 %arg1: memref<4xf64> {onnx.name = "input_x", onnx.type = "encrypted"}) -> memref<4xf64> {
    %c5 = arith.constant 5 : index
    %c4 = arith.constant 4 : index
    %c1 = arith.constant 1 : index
    %c0 = arith.constant 0 : index
    %cst = arith.constant 0.000000e+00 : f64
    %0 = memref.load %arg0[%c0] : memref<16xf64>
    %1 = memref.load %arg1[%c0] : memref<4xf64>
    %2 = arith.mulf %0, %1 : f64
    %3 = arith.addf %2, %cst : f64
    %4 = memref.load %arg0[%c1] : memref<16xf64>
    %5 = memref.load %arg1[%c1] : memref<4xf64>
    %6 = arith.mulf %4, %5 : f64
    %7 = arith.addf %3, %6 : f64
    memref.store %7, %arg1[%c0] : memref<4xf64>
    %8 = memref.load %arg0[%c4] : memref<16xf64>
    %9 = memref.load %arg1[%c0] : memref<4xf64>
    %10 = arith.mulf %8, %9 : f64
    %11 = arith.addf %10, %cst : f64
    %12 = memref.load %arg0[%c5] : memref<16xf64>
    %13 = memref.load %arg1[%c1] : memref<4xf64>
    %14 = arith.mulf %12, %13 : f64
    %15 = arith.addf %11, %14 : f64
    memref.store %15, %arg1[%c1] : memref<4xf64>
    return %arg1 : memref<4xf64>
  }
}


// CHECK: secret.mul
// CHECK: secret.mul
// CHECK: secret.add
// CHECK: secret.mul
// CHECK: secret.mul
// CHECK: secret.add