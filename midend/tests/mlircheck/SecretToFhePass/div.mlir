// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe < %s | FileCheck %s

module {

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32

        // CHECK-NOT: arith.divf
        // CHECK-NOT: secret.div
        // CHECK: fhe.lwediv
    }

    func.func @div_1(%arg0: f32 {onnx.name = "input_x1", onnx.type = "encrypted"}, 
                     %arg1: f32 {onnx.name = "input_y1", onnx.type = "clear"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32

        // CHECK-NOT: arith.divf
        // CHECK-NOT: secret.div_plain
        // CHECK: fhe.lwediv_plain
    }

    func.func @div_2(%arg0: f32 {onnx.name = "input_x2", onnx.type = "clear"}, 
                     %arg1: f32 {onnx.name = "input_y2", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32

        // CHECK-NOT: arith.divf
        // CHECK-NOT: secret.reciprocal
        // CHECK-NOT: secret.mul_plain
        // CHECK: fhe.lwereciprocal
        // CHECK: fhe.lwemul_plain
    }
}