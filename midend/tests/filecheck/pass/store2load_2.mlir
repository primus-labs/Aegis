// RUN: aegiscompiler --forward-store-to-load --canonicalize --cse < %s | FileCheck %s

module {
    func.func @multiple_stores(%arg0: memref<10xf32>) {
        %c0 = arith.constant 0 : index
        %val1 = arith.constant 42.0 : f32
        %val2 = arith.constant 43.0 : f32
        affine.store %val1, %arg0[%c0] : memref<10xf32>
        affine.store %val2, %arg0[%c0] : memref<10xf32>
        %loaded = affine.load %arg0[%c0] : memref<10xf32>
        return
    }
}

// CHECK-NOT: affine.store
// CHECK-NOT: affine.load
