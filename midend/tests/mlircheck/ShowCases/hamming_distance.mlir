// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --canonicalize --cse --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --canonicalize --cse --cast-to-emitc-stub --insert-emitc-preamble --canonicalize --cse  < %s | emitc-translate --mlir-to-cpp | FileCheck %s

module {
    func.func @hamming_distance(%arg0: memref<4xf64>, %arg1: memref<4xf64>) -> f64 {
        %sum_init = arith.constant 0.0 : f64
        
        // Create affine loop with 4 iterations (0 to 3 inclusive)
        %result = affine.for %i = 0 to 4 iter_args(%acc = %sum_init) -> f64 {
            // Load elements from both arrays using affine index
            %a = memref.load %arg0[%i] : memref<4xf64>  // array1[i]
            %b = memref.load %arg1[%i] : memref<4xf64>  // array2[i]
            
            // Compare floating-point values for inequality
            %cmp = arith.cmpf "une", %a, %b : f64  // 'une' = unordered not equal
            
            // Convert comparison result to numerical values
            %one = arith.constant 1.0 : f64  // mismatch value
            %zero = arith.constant 0.0 : f64  // match value
            %delta = arith.select %cmp, %one, %zero : f64
            
            // Accumulate the difference
            %new_acc = arith.addf %acc, %delta : f64
            
            // Pass updated accumulator to next iteration
            affine.yield %new_acc : f64
        }
        
        // Return final accumulated hamming distance
        return %result : f64
        }
}


// CHECK: RLWECipher hamming_distance(RLWECipher v1, RLWECipher v2) {
// CHECK:   PlainVector v3 = MakeMultPlain(0,0,0,1);
// CHECK:   PlainVector v4 = MakeMultPlain(0,0,1,0);
// CHECK:   PlainVector v5 = MakeMultPlain(0,1,0,0);
// CHECK:   Plain v6 = MakePlain(1.000000);
// CHECK:   Plain v7 = MakePlain(0.000000);
// CHECK:   PlainVector v8 = MakeMultPlain(1,0,0,0);
// CHECK:   RLWECipher v9 = MulPlain(v1, v8);
// CHECK:   RLWECipher v10 = MulPlain(v2, v8);
// CHECK:   RLWECipher v11 = Cmp_ue(v9, v10);
// CHECK:   RLWECipher v12 = Cast_Plain_To_Cipher(v6);
// CHECK:   RLWECipher v13 = Cast_Plain_To_Cipher(v7);
// CHECK:   RLWECipher v14 = Select(v11, v12, v13);
// CHECK:   RLWECipher v15 = MulPlain(v1, v5);
// CHECK:   RLWECipher v16 = MulPlain(v2, v5);
// CHECK:   RLWECipher v17 = Cmp_ue(v15, v16);
// CHECK:   RLWECipher v18 = Select(v17, v12, v13);
// CHECK:   RLWECipher v19 = Add(v14, v18);
// CHECK:   RLWECipher v20 = MulPlain(v1, v4);
// CHECK:   RLWECipher v21 = MulPlain(v2, v4);
// CHECK:   RLWECipher v22 = Cmp_ue(v20, v21);
// CHECK:   RLWECipher v23 = Select(v22, v12, v13);
// CHECK:   RLWECipher v24 = Add(v19, v23);
// CHECK:   RLWECipher v25 = MulPlain(v1, v3);
// CHECK:   RLWECipher v26 = MulPlain(v2, v3);
// CHECK:   RLWECipher v27 = Cmp_ue(v25, v26);
// CHECK:   RLWECipher v28 = Select(v27, v12, v13);
// CHECK:   RLWECipher v29 = Add(v24, v28);
// CHECK:   return v29;
// CHECK: }