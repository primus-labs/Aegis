// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse --lwe-to-rlwe -canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub < %s | FileCheck %s

module {
    func.func @euclidean_distance(%arg0: memref<4xf64>, %arg1: memref<4xf64>) -> f64 {
        %sum_init = arith.constant 0.0 : f64
        
        // Create affine loop with 4 iterations (0 to 3 inclusive)
        %result = affine.for %i = 0 to 4 iter_args(%acc = %sum_init) -> f64 {
            %1 = memref.load %arg0[%i] : memref<4xf64>  // array1[i]
            %2 = memref.load %arg1[%i] : memref<4xf64>  // array2[i]
            %3 = arith.subf %1, %2 : f64

            %4 = memref.load %arg0[%i] : memref<4xf64>  // array1[i]
            %5 = memref.load %arg1[%i] : memref<4xf64>  // array2[i]
            %6 = arith.subf %4, %5 : f64

            %7 = arith.mulf %3, %6 : f64
            %8 = arith.addf %acc, %7 : f64
            
            // Pass updated accumulator to next iteration
            affine.yield %8 : f64
        }
        
        // Return final accumulated euclidean distance
        return %result : f64
    }
}


// CHECK: emitc.call_opaque "Sub"
// CHECK: emitc.call_opaque "Mul"
// CHECK: emitc.call_opaque "Rotate"
// CHECK: emitc.call_opaque "Add"
// CHECK: emitc.call_opaque "Rotate"
// CHECK: emitc.call_opaque "Add"
// CHECK: emitc.call_opaque "Rotate"
// CHECK: emitc.call_opaque "Add"
// CHECK: emitc.call_opaque "MulPlain"
// CHECK: return