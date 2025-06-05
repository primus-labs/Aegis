// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --canonicalize --cse --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --lower-multi-dim-load --canonicalize --cse --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --canonicalize --cse --cast-to-emitc-stub --insert-emitc-preamble --canonicalize --cse  < %s | emitc-translate --mlir-to-cpp | FileCheck %s


module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "add"} {
  func.func @main_graph(%arg0: memref<3x2xf32> {onnx.name = "X1"}, %arg1: memref<3x2xf32> {onnx.name = "X2"}) -> (memref<3x2xf32> {onnx.name = "Y"}) attributes {llvm.emit_c_interface} {
    %alloc = memref.alloc() {alignment = 16 : i64} : memref<3x2xf32>
    affine.for %arg2 = 0 to 3 {
      affine.for %arg3 = 0 to 2 {
        %0 = affine.load %arg0[%arg2, %arg3] : memref<3x2xf32>
        %1 = affine.load %arg1[%arg2, %arg3] : memref<3x2xf32>
        %2 = arith.addf %0, %1 : f32
        affine.store %2, %alloc[%arg2, %arg3] : memref<3x2xf32>
      }
    }
    return %alloc : memref<3x2xf32>
  }
}


//CHECK: std::vector<RLWECipher> main_graph(std::vector<RLWECipher> v1, std::vector<RLWECipher> v2) {
//CHECK:   PlainVector v3 = MakeMultPlain
//CHECK:   PlainVector v4 = MakeMultPlain
//CHECK:   PlainVector v5 = MakeMultPlain
//CHECK:   PlainVector v6 = MakeMultPlain
//CHECK:   Plain v7 = MakePlain(2);
//CHECK:   Plain v8 = MakePlain(1);
//CHECK:   Plain v9 = MakePlain(0);
//CHECK:   size_t v10 = Cast_Plain_To_Index(v9);
//CHECK:   size_t v11 = Cast_Plain_To_Index(v8);
//CHECK:   size_t v12 = Cast_Plain_To_Index(v7);
//CHECK:   std::vector<RLWECipher> v13 = AllocArray(3, 16);
//CHECK:   RLWECipher v14 = Vload(v1, v10);
//CHECK:   RLWECipher v15 = Vload(v2, v10);
//CHECK:   RLWECipher v16 = Add(v14, v15);
//CHECK:   RLWECipher v17 = MulPlain(v16, v6);
//CHECK:   RLWECipher v18 = Vload(v13, v10);
//CHECK:   RLWECipher v19 = MulPlain(v18, v5);
//CHECK:   RLWECipher v20 = Add(v19, v17);
//CHECK:   Vstore(v20, v13, v10);
//CHECK:   RLWECipher v21 = Vload(v1, v10);
//CHECK:   RLWECipher v22 = Vload(v2, v10);
//CHECK:   RLWECipher v23 = Add(v21, v22);
//CHECK:   RLWECipher v24 = MulPlain(v23, v4);
//CHECK:   RLWECipher v25 = Vload(v13, v10);
//CHECK:   RLWECipher v26 = MulPlain(v25, v3);
//CHECK:   RLWECipher v27 = Add(v26, v24);
//CHECK:   Vstore(v27, v13, v10);
//CHECK:   RLWECipher v28 = Vload(v1, v11);
//CHECK:   RLWECipher v29 = Vload(v2, v11);
//CHECK:   RLWECipher v30 = Add(v28, v29);
//CHECK:   RLWECipher v31 = MulPlain(v30, v6);
//CHECK:   RLWECipher v32 = Vload(v13, v11);
//CHECK:   RLWECipher v33 = MulPlain(v32, v5);
//CHECK:   RLWECipher v34 = Add(v33, v31);
//CHECK:   Vstore(v34, v13, v11);
//CHECK:   RLWECipher v35 = Vload(v1, v11);
//CHECK:   RLWECipher v36 = Vload(v2, v11);
//CHECK:   RLWECipher v37 = Add(v35, v36);
//CHECK:   RLWECipher v38 = MulPlain(v37, v4);
//CHECK:   RLWECipher v39 = Vload(v13, v11);
//CHECK:   RLWECipher v40 = MulPlain(v39, v3);
//CHECK:   RLWECipher v41 = Add(v40, v38);
//CHECK:   Vstore(v41, v13, v11);
//CHECK:   RLWECipher v42 = Vload(v1, v12);
//CHECK:   RLWECipher v43 = Vload(v2, v12);
//CHECK:   RLWECipher v44 = Add(v42, v43);
//CHECK:   RLWECipher v45 = MulPlain(v44, v6);
//CHECK:   RLWECipher v46 = Vload(v13, v12);
//CHECK:   RLWECipher v47 = MulPlain(v46, v5);
//CHECK:   RLWECipher v48 = Add(v47, v45);
//CHECK:   Vstore(v48, v13, v12);
//CHECK:   RLWECipher v49 = Vload(v1, v12);
//CHECK:   RLWECipher v50 = Vload(v2, v12);
//CHECK:   RLWECipher v51 = Add(v49, v50);
//CHECK:   RLWECipher v52 = MulPlain(v51, v4);
//CHECK:   RLWECipher v53 = Vload(v13, v12);
//CHECK:   RLWECipher v54 = MulPlain(v53, v3);
//CHECK:   RLWECipher v55 = Add(v54, v52);
//CHECK:   Vstore(v55, v13, v12);
//CHECK:   return v13;
//CHECK: }