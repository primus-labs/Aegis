// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --canonicalize --cse --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --canonicalize --cse --cast-to-emitc-stub --insert-emitc-preamble --canonicalize --cse  < %s | emitc-translate --mlir-to-cpp | FileCheck %s


module {
  func.func @encryptedRobertsCross_64x64(%arg0: memref<4096xf32>, %arg1: memref<4096xf32>) -> memref<4096xf32> {
    affine.for %arg2 = 0 to 64 {
      affine.for %arg3 = 0 to 64 {
        %0 = affine.load %arg0[((%arg2 - 1) * 64 + %arg3 - 1) mod 4096] : memref<4096xf32>
        %1 = affine.load %arg0[(%arg2 * 64 + %arg3) mod 4096] : memref<4096xf32>
        %2 = affine.load %arg0[((%arg2 - 1) * 64 + %arg3) mod 4096] : memref<4096xf32>
        %3 = affine.load %arg0[(%arg2 * 64 + %arg3 - 1) mod 4096] : memref<4096xf32>
        %4 = arith.subf %0, %1 : f32
        %5 = arith.mulf %4, %4 : f32
        %6 = arith.subf %2, %3 : f32
        %7 = arith.mulf %6, %6 : f32
        %8 = arith.addf %5, %7 : f32
        affine.store %8, %arg1[(%arg2 * 64 + %arg3) mod 4096] : memref<4096xf32>
      }
    }
    return %arg1 : memref<4096xf32>
  }
}


//CHECK: RLWECipher encryptedRobertsCross_64x64(RLWECipher v1, RLWECipher v2) {
//CHECK:   RLWECipher v3 = Rotate(v1, -4031);
//CHECK:   RLWECipher v4 = Sub(v1, v3);
//CHECK:   RLWECipher v5 = Mul(v4, v4);
//CHECK:   RLWECipher v6 = Rotate(v1, -4033);
//CHECK:   RLWECipher v7 = Sub(v1, v6);
//CHECK:   RLWECipher v8 = Mul(v7, v7);
//CHECK:   RLWECipher v9 = Rotate(v5, -65);
//CHECK:   RLWECipher v10 = Rotate(v8, -64);
//CHECK:   RLWECipher v11 = Add(v9, v10);
//CHECK:   Copy(v11, v2);
//CHECK:   return v2;
//CHECK: }
