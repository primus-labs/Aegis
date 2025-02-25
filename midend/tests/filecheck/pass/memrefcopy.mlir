// RUN: aegiscompiler -expand-memref-copy --canonicalize --cse < %s | FileCheck %s

module {
    func.func @testcase() {
        %alloc = memref.alloc() : memref<2x3xi32>
        %alloc_0 = memref.alloc() : memref<2x3xi32>
        memref.copy %alloc, %alloc_0 : memref<2x3xi32> to memref<2x3xi32>
        func.return
    }
}

// CHECK-NOT: memref.copy
