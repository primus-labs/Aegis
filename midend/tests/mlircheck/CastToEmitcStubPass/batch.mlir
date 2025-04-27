// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --lwe-to-rlwe -canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub < %s | FileCheck %s


module  {
  func.func  @MVP(%m: memref<16xf64> {onnx.name = "input_x", onnx.type = "clear"}, 
                  %v: memref<4xf64>  {onnx.name = "input_y", onnx.type = "encrypted"}
                  ) -> memref<4xf64> {
    %c0 = arith.constant 0 : index
    %c4 = arith.constant 4 : index
    %c0_sf64 = arith.constant 0.000000e+00 : f64
    // for each row in matrix
    %0 = affine.for %i = 0 to 2 iter_args(%r = %v) -> (memref<4xf64>) {
      // iterate over the vector
      %1 = affine.for %j = 0 to 2 iter_args(%sum = %c0_sf64) -> (f64) {
       // compute ij = i*4 + j
       %2 = arith.muli %i, %c4 : index
       %ij = arith.addi %2, %j : index
        %mij = memref.load %m[%ij] : memref<16xf64>
        %vj = memref.load %v[%j] : memref<4xf64>
        %p = arith.mulf %mij, %vj  : f64
        %s = arith.addf %sum, %p : f64
        affine.yield %s : f64
      }

      memref.store %1, %r[%i] : memref<4xf64>
      affine.yield %r : memref<4xf64>
    }
    return %0: memref<4xf64>
  }
}



//CHECK: "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
//CHECK: "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
//CHECK: "emitc.constant"() <{value = #emitc.opaque<"MakePlain(0)">}> : () -> !emitc.opaque<"Plain">
//CHECK: "emitc.constant"() <{value = #emitc.opaque<"MakePlain(1)">}> : () -> !emitc.opaque<"Plain">
//CHECK: "emitc.constant"() <{value = #emitc.opaque<"MakePlain(4)">}> : () -> !emitc.opaque<"Plain">
//CHECK: "emitc.constant"() <{value = #emitc.opaque<"MakePlain(5)">}> : () -> !emitc.opaque<"Plain">
//CHECK: emitc.call_opaque "Cast_Plain_To_Index"
//CHECK: emitc.call_opaque "Cast_Plain_To_Index"
//CHECK: emitc.call_opaque "Cast_Plain_To_Index"
//CHECK: emitc.call_opaque "Cast_Plain_To_Index"
//CHECK: emitc.call_opaque "Native_Load"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Native_Load"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Rotate"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Copy"
//CHECK: emitc.call_opaque "Native_Load"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Native_Load"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Rotate"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Rotate"
//CHECK: emitc.call_opaque "MulPlain"
//CHECK: emitc.call_opaque "Add"
//CHECK: emitc.call_opaque "Copy"
