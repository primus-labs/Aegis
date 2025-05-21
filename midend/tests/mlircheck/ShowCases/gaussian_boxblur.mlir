// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --canonicalize --cse --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fold-arith-chain --canonicalize --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --canonicalize --cse --cast-to-emitc-stub --insert-emitc-preamble --canonicalize --cse  < %s | emitc-translate --mlir-to-cpp | FileCheck %s


module {
  func.func @encryptedBoxBlur_64x64(%arg0: memref<4096xf32>, %arg1: memref<4096xf32>) -> memref<4096xf32> {
    affine.for %arg2 = 0 to 64 {
      affine.for %arg3 = 0 to 64 {
        %cst = arith.constant 0.000000e+00 : f32
        %0 = affine.for %arg4 = 0 to 3 iter_args(%arg5 = %cst) -> (f32) {
          %1 = affine.for %arg6 = 0 to 3 iter_args(%arg7 = %arg5) -> (f32) {
            %2 = affine.load %arg0[((%arg2 + %arg6 - 1) * 64 + %arg3 + %arg4 - 1) mod 4096] : memref<4096xf32>
            %3 = arith.addf %arg7, %2 : f32
            affine.yield %3 : f32
          }
          affine.yield %1 : f32
        }
        affine.store %0, %arg1[(%arg2 * 64 + %arg3) mod 4096] : memref<4096xf32>
      }
    }
    return %arg1 : memref<4096xf32>
  }
}


//CHECK: RLWECipher encryptedBoxBlur_64x64(RLWECipher v1, RLWECipher v2) {
//CHECK:   RLWECipher v3 = Rotate(v1, -4031);
//CHECK:   RLWECipher v4 = Rotate(v1, -4095);
//CHECK:   RLWECipher v5 = Rotate(v1, -63);
//CHECK:   RLWECipher v6 = Rotate(v1, -4032);
//CHECK:   RLWECipher v7 = Rotate(v1, -64);
//CHECK:   RLWECipher v8 = Rotate(v1, -4033);
//CHECK:   RLWECipher v9 = Rotate(v1, -65);
//CHECK:   RLWECipher v10 = Rotate(v1, -1);
//CHECK:   RLWECipher v11 = Add(v3, v4, v5, v6, v1, v7, v8, v9, v10);
//CHECK:   Copy(v11, v2);
//CHECK:   return v2;
//CHECK: }
