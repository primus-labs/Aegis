// RUN: aegiscompiler --forward-insert-to-extract --canonicalize --cse < %s | FileCheck %s

module {
    func.func @test_no_forwarding(%arg0: tensor<4xi32>, %arg1: index, %arg2: i32, %arg3: i32) -> i32 {
        %0 = tensor.insert %arg2 into %arg0[%arg1] : tensor<4xi32>
        %1 = tensor.insert %arg3 into %0[%arg1] : tensor<4xi32>
        %2 = tensor.extract %1[%arg1] : tensor<4xi32>
        return %2 : i32
    }
}

// CHECK-NOT: tensor.insert
// CHECK-NOT: tensor.extract