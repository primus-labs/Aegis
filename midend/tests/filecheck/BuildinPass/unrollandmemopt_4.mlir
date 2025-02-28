// RUN: aegiscompiler -unroll-loop-and-memory-opt --canonicalize --cse < %s | FileCheck %s

module {
    // CHECK-LABEL: func @test_subview_collapse
    func.func @test_subview_collapse(%arg0: memref<10xf32>) -> f32 {
        // CHECK-NOT: affine.load
        // CHECK: return %{{.*}} : f32

        // Create a subview of the memref.
        %c0 = arith.constant 0 : index
        %subview = memref.subview %arg0[%c0][5][1] : memref<10xf32> to memref<5xf32, strided<[1], offset: ?>>

        // Store a value to the subview.
        %cst = arith.constant 42.0 : f32
        affine.store %cst, %subview[%c0] : memref<5xf32, strided<[1], offset: ?>>

        // Load the value from the subview.
        %val = affine.load %subview[%c0] : memref<5xf32, strided<[1], offset: ?>>

        // The load should be replaced with the stored value.
        return %val : f32
    }
}
