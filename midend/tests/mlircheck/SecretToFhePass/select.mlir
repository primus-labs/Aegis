// RUN: aegiscompiler -collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe < %s | FileCheck %s

module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %cond = arith.cmpf "olt", %arg0, %arg1 : f32
        %result = arith.select %cond, %arg0, %arg1 : f32
        return %result : f32
    }
}

// CHECK-NOT: arith.cmpf
// CHECK-NOT: arith.select
// CHECK-NOT: secret.compare
// CHECK-NOT: secret.select
// CHECK: fhe.compare
// CHECK: fhe.select