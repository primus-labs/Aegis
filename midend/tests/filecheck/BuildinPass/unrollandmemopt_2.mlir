// RUN: aegiscompiler -unroll-loop-and-memory-opt --canonicalize --cse < %s | FileCheck %s

module {
    // CHECK-LABEL: func @test_unroll_and_mem_opt
    func.func @test_unroll_and_mem_opt(%arg0: memref<10xf32>) -> f32 {
        // CHECK-NOT: affine.for
        // CHECK-NOT: affine.load
        // CHECK: return %{{.*}} : f32

        // Allocate a memref.
        %mem = memref.alloc() : memref<10xf32>

        // Store a value to memref inside a loop.
        affine.for %i = 0 to 10 {
            %cst = arith.constant 42.0 : f32
            affine.store %cst, %mem[%i] : memref<10xf32>
        }

        // Load the value from memref inside a loop.
        %sum_init = arith.constant 0.0 : f32 
        %sum = affine.for %i = 0 to 10 iter_args(%sum_iter = %sum_init) -> f32 {
            %val = affine.load %mem[%i] : memref<10xf32>
            %sum_next = arith.addf %sum_iter, %val : f32  
            affine.yield %sum_next : f32  
        }

        // The loop should be unrolled, and the load should be replaced with the stored value.
        return %sum : f32
    }
}