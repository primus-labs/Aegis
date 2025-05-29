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
//CHECK:   PlainVector v3 = MakeMultPlain(0,1);
//CHECK:   PlainVector v4 = MakeMultPlain(1,0);
//CHECK:   Plain v5 = MakePlain(2);
//CHECK:   Plain v6 = MakePlain(1);
//CHECK:   Plain v7 = MakePlain(0);
//CHECK:   size_t v8 = Cast_Plain_To_Index(v7);
//CHECK:   size_t v9 = Cast_Plain_To_Index(v6);
//CHECK:   size_t v10 = Cast_Plain_To_Index(v5);
//CHECK:   std::vector<RLWECipher> v11 = AllocArray(3, 16);
//CHECK:   RLWECipher v12 = Vload(v1, v8);
//CHECK:   RLWECipher v13 = Vload(v2, v8);
//CHECK:   RLWECipher v14 = Add(v12, v13);
//CHECK:   RLWECipher v15 = MulPlain(v14, v4);
//CHECK:   RLWECipher v16 = Vload(v11, v8);
//CHECK:   RLWECipher v17 = MulPlain(v16, v3);
//CHECK:   RLWECipher v18 = Add(v17, v15);
//CHECK:   Vstore(v18, v11, v8);
//CHECK:   RLWECipher v19 = Vload(v1, v8);
//CHECK:   RLWECipher v20 = Vload(v2, v8);
//CHECK:   RLWECipher v21 = Add(v19, v20);
//CHECK:   RLWECipher v22 = MulPlain(v21, v3);
//CHECK:   RLWECipher v23 = Vload(v11, v8);
//CHECK:   RLWECipher v24 = MulPlain(v23, v4);
//CHECK:   RLWECipher v25 = Add(v24, v22);
//CHECK:   Vstore(v25, v11, v8);
//CHECK:   RLWECipher v26 = Vload(v1, v9);
//CHECK:   RLWECipher v27 = Vload(v2, v9);
//CHECK:   RLWECipher v28 = Add(v26, v27);
//CHECK:   RLWECipher v29 = MulPlain(v28, v4);
//CHECK:   RLWECipher v30 = Vload(v11, v9);
//CHECK:   RLWECipher v31 = MulPlain(v30, v3);
//CHECK:   RLWECipher v32 = Add(v31, v29);
//CHECK:   Vstore(v32, v11, v9);
//CHECK:   RLWECipher v33 = Vload(v1, v9);
//CHECK:   RLWECipher v34 = Vload(v2, v9);
//CHECK:   RLWECipher v35 = Add(v33, v34);
//CHECK:   RLWECipher v36 = MulPlain(v35, v3);
//CHECK:   RLWECipher v37 = Vload(v11, v9);
//CHECK:   RLWECipher v38 = MulPlain(v37, v4);
//CHECK:   RLWECipher v39 = Add(v38, v36);
//CHECK:   Vstore(v39, v11, v9);
//CHECK:   RLWECipher v40 = Vload(v1, v10);
//CHECK:   RLWECipher v41 = Vload(v2, v10);
//CHECK:   RLWECipher v42 = Add(v40, v41);
//CHECK:   RLWECipher v43 = MulPlain(v42, v4);
//CHECK:   RLWECipher v44 = Vload(v11, v10);
//CHECK:   RLWECipher v45 = MulPlain(v44, v3);
//CHECK:   RLWECipher v46 = Add(v45, v43);
//CHECK:   Vstore(v46, v11, v10);
//CHECK:   RLWECipher v47 = Vload(v1, v10);
//CHECK:   RLWECipher v48 = Vload(v2, v10);
//CHECK:   RLWECipher v49 = Add(v47, v48);
//CHECK:   RLWECipher v50 = MulPlain(v49, v3);
//CHECK:   RLWECipher v51 = Vload(v11, v10);
//CHECK:   RLWECipher v52 = MulPlain(v51, v4);
//CHECK:   RLWECipher v53 = Add(v52, v50);
//CHECK:   Vstore(v53, v11, v10);
//CHECK:   return v11;
//CHECK: }