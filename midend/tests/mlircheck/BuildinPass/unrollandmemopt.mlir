// RUN: aegiscompiler -unroll-loop-and-memory-opt --canonicalize --cse < %s | FileCheck %s

module {
    // CHECK-LABEL: func @test_forward_store_to_load
    func.func @test_forward_store_to_load(%arg0: memref<10xf32>) -> f32 {
        // CHECK-NOT: affine.load
        // CHECK: return %{{.*}} : f32

        // Store a value to memref.
        %c0 = arith.constant 0 : index
        %cst = arith.constant 42.0 : f32
        affine.store %cst, %arg0[%c0] : memref<10xf32>

        // Load the value from the same location.
        %val = affine.load %arg0[%c0] : memref<10xf32>

        // The load should be replaced with the stored value.
        return %val : f32
    }
}