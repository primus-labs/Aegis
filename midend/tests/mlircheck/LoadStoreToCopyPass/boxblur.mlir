// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --canonicalize --cse  --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fold-arith-chain --canonicalize --cse --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse < %s | FileCheck %s


module {
  func.func @BoxBlur_4x4(%arg0: memref<16xf32>, %arg1: memref<16xf32>) -> memref<16xf32> {
    affine.for %arg2 = 0 to 4 {
      affine.for %arg3 = 0 to 4 {
        %cst = arith.constant 0.000000e+00 : f32
        %0 = affine.for %arg4 = 0 to 3 iter_args(%arg5 = %cst) -> (f32) {
          %1 = affine.for %arg6 = 0 to 3 iter_args(%arg7 = %arg5) -> (f32) {
            %2 = affine.load %arg0[((%arg2 + %arg6 - 1) * 4 + %arg3 + %arg4 - 1) mod 16] : memref<16xf32>
            %3 = arith.addf %arg7, %2 : f32
            affine.yield %3 : f32
          }
          affine.yield %1 : f32
        }
        affine.store %0, %arg1[(%arg2 * 4 + %arg3) mod 16] : memref<16xf32>
      }
    }
    return %arg1 : memref<16xf32>
  }
}


//CHECK-NOT: fhe.load
//CHECK-NOT: fhe.store
//CHECK: fhe.copy

