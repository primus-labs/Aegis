// RUN: aegiscompiler -forward-insert-to-extract --canonicalize --cse < %s | FileCheck %s

module {
    func.func @test_forward_insert_to_extract(%arg0: tensor<4xi32>, %arg1: index, %arg2: i32) -> i32 {
        %0 = tensor.insert %arg2 into %arg0[%arg1] : tensor<4xi32>
        %1 = tensor.extract %0[%arg1] : tensor<4xi32>
        return %1 : i32
    }
}

// CHECK-NOT: tensor.insert
// CHECK-NOT: tensor.extract