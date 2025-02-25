// RUN: aegiscompiler --forward-store-to-load --canonicalize --cse < %s | FileCheck %s

module {
    func.func @simple_store_load(%arg0: memref<10xf32>) {
        %c0 = arith.constant 0 : index
        %c1 = arith.constant 1 : index
        %val = arith.constant 42.0 : f32
        affine.store %val, %arg0[%c0] : memref<10xf32>
        %loaded = affine.load %arg0[%c0] : memref<10xf32>
        return
    }
}

// CHECK-NOT: affine.store
// CHECK-NOT: affine.load