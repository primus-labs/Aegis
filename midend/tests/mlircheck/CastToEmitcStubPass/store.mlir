// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse --lwe-to-rlwe -canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub < %s | FileCheck %s


module {
  func.func @main_graph(%arg0: memref<1xf32> , %arg1: memref<1xf32> ) -> (memref<1xf32> ) {
    %c0 = arith.constant 0 : index
    %alloc = memref.alloc() {alignment = 16 : i64} : memref<1xf32>
    %0 = affine.load %arg0[%c0] : memref<1xf32>
    %1 = affine.load %arg1[%c0] : memref<1xf32>
    %2 = arith.addf %0, %1 : f32
    affine.store %2, %alloc[%c0] : memref<1xf32>
    return %alloc : memref<1xf32>
  }
}


// CHECK: emitc.call_opaque "Alloc"()
// CHECK: emitc.call_opaque "Add"
// CHECK: emitc.call_opaque "Copy"
// CHECK: return