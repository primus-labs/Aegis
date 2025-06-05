// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --canonicalize --cse --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --lower-multi-dim-load --canonicalize --cse --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --canonicalize --cse  < %s | FileCheck %s

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


//CHECK: emitc.call_opaque "Cast_Plain_To_Index"
//CHECK: emitc.call_opaque "Cast_Plain_To_Index"
//CHECK: emitc.call_opaque "Cast_Plain_To_Index"
//CHECK: emitc.call_opaque "AllocArray"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Vstore"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Vstore"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Vstore"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Vstore"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Vstore"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Vload"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Vstore"
