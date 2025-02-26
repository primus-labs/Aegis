// RUN: aegiscompiler --forward-store-to-load --canonicalize --cse < %s | FileCheck %s

module {
    func.func @multiple_stores(%arg0: memref<10xf32>, %cond: i1) -> f32  {
        %c0 = arith.constant 0 : index
        %val = arith.constant 42.0 : f32
        affine.store %val, %arg0[%c0] : memref<10xf32>
        cf.cond_br %cond, ^bb1, ^bb2
        ^bb1:
            %loaded = affine.load %arg0[%c0] : memref<10xf32>
            return %loaded : f32
        ^bb2:
            %loaded_1 = affine.load %arg0[1] : memref<10xf32>
            return %loaded_1 : f32
    }
}

// CHECK: memref.store
// CHECK: memref.load
