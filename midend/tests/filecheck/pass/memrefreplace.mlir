 // RUN: aegiscompiler -global-memref-replace --canonicalize --cse < %s | FileCheck %s
 module {
    memref.global "private" constant @__constant_8xi16 : memref<2x4xi16> = dense<[[1, 2, 3, 4], [5, 6, 7, 8]]>
    func.func @main() -> i16 {
        %c1 = arith.constant 1 : index
        %c2 = arith.constant 3 : index
        %0 = memref.get_global @__constant_8xi16 : memref<2x4xi16>
        %1 = affine.load %0[%c1, %c1 + %c2] : memref<2x4xi16>
        return %1 : i16
    }
}

// CHECK-NOT: memref.global