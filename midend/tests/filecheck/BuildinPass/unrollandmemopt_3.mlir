// RUN: aegiscompiler -unroll-loop-and-memory-opt --canonicalize --cse < %s | FileCheck %s

module {
    // CHECK-LABEL: func @test_no_store_found
    func.func @test_no_store_found(%arg0: memref<10xf32>) -> f32 {
        // CHECK: affine.load
        // CHECK: return %{{.*}} : f32

        // Load from a location that has no corresponding store.
        %c1 = arith.constant 1 : index
        %val = affine.load %arg0[%c1] : memref<10xf32>

        // The load should remain unchanged.
        return %val : f32
    }
}