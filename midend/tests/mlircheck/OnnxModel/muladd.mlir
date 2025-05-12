// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --canonicalize --cse --cast-to-emitc-stub --insert-emitc-preamble --canonicalize --cse  < %s | emitc-translate --mlir-to-cpp | FileCheck %s


// convert muladd_model.onnx to muladd.mlir using onnx-mlir
// #map = affine_map<(d0) -> (d0)>
// module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "addmul_model"} {
//   func.func @main_graph(%arg0: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_x"}, %arg1: memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "input_y"}) -> (memref<?xf32> {onnx.dim_params = "0:batch_size", onnx.name = "output"}) attributes {llvm.emit_c_interface} {
//     %c0 = arith.constant 0 : index
//     %0 = "krnl.global"() {name = "constant_0", shape = [], value = dense<2.000000e+00> : tensor<f32>} : () -> memref<f32>
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
//   "krnl.entry_point"() {func = @main_graph, numInputs = 2 : i32, numOutputs = 1 : i32, signature = "[    { \22type\22 : \22f32\22 , \22dims\22 : [-1] , \22name\22 : \22input_x\22 }\0A ,    { \22type\22 : \22f32\22 , \22dims\22 : [-1] , \22name\22 : \22input_y\22 }\0A\0A]\00@[   { \22type\22 : \22f32\22 , \22dims\22 : [-1] , \22name\22 : \22output\22 }\0A\0A]\00"} : () -> ()
// }


// Make minimal manual modifications.
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



// CHECK: extern float constant_0[1] = {2.000000000e+00f};
// CHECK: RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
// CHECK:   PlainVector v3 = MakeMultPlain(1,1,1,1,1,1,1,0);
// CHECK:   PlainVector v4 = MakeMultPlain(0,0,0,0,0,0,0,1);
// CHECK:   PlainVector v5 = MakeMultPlain(1,1,1,1,1,1,0,1);
// CHECK:   PlainVector v6 = MakeMultPlain(0,0,0,0,0,0,1,0);
// CHECK:   PlainVector v7 = MakeMultPlain(1,1,1,1,1,0,1,1);
// CHECK:   PlainVector v8 = MakeMultPlain(0,0,0,0,0,1,0,0);
// CHECK:   PlainVector v9 = MakeMultPlain(1,1,1,1,0,1,1,1);
// CHECK:   PlainVector v10 = MakeMultPlain(0,0,0,0,1,0,0,0);
// CHECK:   PlainVector v11 = MakeMultPlain(1,1,1,0,1,1,1,1);
// CHECK:   PlainVector v12 = MakeMultPlain(0,0,0,1,0,0,0,0);
// CHECK:   PlainVector v13 = MakeMultPlain(1,1,0,1,1,1,1,1);
// CHECK:   PlainVector v14 = MakeMultPlain(0,0,1,0,0,0,0,0);
// CHECK:   PlainVector v15 = MakeMultPlain(1,0,1,1,1,1,1,1);
// CHECK:   PlainVector v16 = MakeMultPlain(0,1,0,0,0,0,0,0);
// CHECK:   PlainVector v17 = MakeMultPlain(0,1,1,1,1,1,1,1);
// CHECK:   PlainVector v18 = MakeMultPlain(1,0,0,0,0,0,0,0);
// CHECK:   PlainVector v19 = ConstantArray_to_PlainVector(constant_0);
// CHECK:   Plain v20 = LoadPlainWithoutIndex(v19);
// CHECK:   RLWECipher v21 = MulPlain(v2, v20);
// CHECK:   RLWECipher v22 = Alloc();
// CHECK:   RLWECipher v23 = Add(v1, v21);
// CHECK:   RLWECipher v24 = MulPlain(v23, v18);
// CHECK:   RLWECipher v25 = MulPlain(v22, v17);
// CHECK:   RLWECipher v26 = Add(v25, v24);
// CHECK:   Copy(v26, v22);
// CHECK:   RLWECipher v27 = Rotate(v21, -1);
// CHECK:   RLWECipher v28 = Add(v1, v27);
// CHECK:   RLWECipher v29 = MulPlain(v28, v16);
// CHECK:   RLWECipher v30 = MulPlain(v22, v15);
// CHECK:   RLWECipher v31 = Add(v30, v29);
// CHECK:   Copy(v31, v22);
// CHECK:   RLWECipher v32 = Rotate(v21, -2);
// CHECK:   RLWECipher v33 = Add(v1, v32);
// CHECK:   RLWECipher v34 = MulPlain(v33, v14);
// CHECK:   RLWECipher v35 = MulPlain(v22, v13);
// CHECK:   RLWECipher v36 = Add(v35, v34);
// CHECK:   Copy(v36, v22);
// CHECK:   RLWECipher v37 = Rotate(v21, -3);
// CHECK:   RLWECipher v38 = Add(v1, v37);
// CHECK:   RLWECipher v39 = MulPlain(v38, v12);
// CHECK:   RLWECipher v40 = MulPlain(v22, v11);
// CHECK:   RLWECipher v41 = Add(v40, v39);
// CHECK:   Copy(v41, v22);
// CHECK:   RLWECipher v42 = Rotate(v21, -4);
// CHECK:   RLWECipher v43 = Add(v1, v42);
// CHECK:   RLWECipher v44 = MulPlain(v43, v10);
// CHECK:   RLWECipher v45 = MulPlain(v22, v9);
// CHECK:   RLWECipher v46 = Add(v45, v44);
// CHECK:   Copy(v46, v22);
// CHECK:   RLWECipher v47 = Rotate(v21, -5);
// CHECK:   RLWECipher v48 = Add(v1, v47);
// CHECK:   RLWECipher v49 = MulPlain(v48, v8);
// CHECK:   RLWECipher v50 = MulPlain(v22, v7);
// CHECK:   RLWECipher v51 = Add(v50, v49);
// CHECK:   Copy(v51, v22);
// CHECK:   RLWECipher v52 = Rotate(v21, -6);
// CHECK:   RLWECipher v53 = Add(v1, v52);
// CHECK:   RLWECipher v54 = MulPlain(v53, v6);
// CHECK:   RLWECipher v55 = MulPlain(v22, v5);
// CHECK:   RLWECipher v56 = Add(v55, v54);
// CHECK:   Copy(v56, v22);
// CHECK:   RLWECipher v57 = Rotate(v21, -7);
// CHECK:   RLWECipher v58 = Add(v1, v57);
// CHECK:   RLWECipher v59 = MulPlain(v58, v4);
// CHECK:   RLWECipher v60 = MulPlain(v22, v3);
// CHECK:   RLWECipher v61 = Add(v60, v59);
// CHECK:   Copy(v61, v22);
// CHECK:   return v22;
// CHECK: }



