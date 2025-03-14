// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse < %s | FileCheck %s

module {

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> i1 {
        %result = arith.cmpf "oeq", %arg0, %arg1 : f32
        return %result : i1
    }
}

// CHECK-LABEL: func.func @main_graph
// CHECK: (%arg0: !secret.secret<f32> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !secret.secret<f32> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !secret.secret<f32>