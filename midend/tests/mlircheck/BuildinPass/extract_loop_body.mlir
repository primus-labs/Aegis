// RUN: aegiscompiler --extract-loop-body --canonicalize --cse < %s | FileCheck %s

module {
    func.func @loop_body() {
        %arg0 = arith.constant -128 : i8
        %c-128_i8 = arith.constant -128 : i8
        %c127_i8 = arith.constant 127 : i8
        %alloc_6 = memref.alloc() {alignment = 64 : i64} : memref<10x20xi8>
        %alloc_7 = memref.alloc() {alignment = 64 : i64} : memref<10x20xi8>
        affine.for %arg1 = 0 to 10 {
            affine.for %arg2 = 0 to 20 {
                %98 = affine.load %alloc_6[%arg1, %arg2] : memref<10x20xi8>
                %99 = arith.cmpi slt, %arg0, %c-128_i8 : i8
                %100 = arith.select %99, %c-128_i8, %arg0 : i8
                %101 = arith.cmpi sgt, %arg0, %c127_i8 : i8
                %102 = arith.select %101, %c127_i8, %100 : i8
                affine.store %102, %alloc_7[%arg1, %arg2] : memref<10x20xi8>
            }
        }

        func.return
    }
}


// CHECK: func.func @for_{{[0-9]+}}(%arg0: i8) -> i8
// CHECK: func.func @loop_body