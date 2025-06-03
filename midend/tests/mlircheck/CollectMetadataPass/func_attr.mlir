// RUN: aegiscompiler --collect-metadata < %s | FileCheck %s
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        return %1 : f32
    }

    func.func @test2(%arg0: f32 {onnx.name = "input_x_2", onnx.type = "clear"}, %arg1: f32 {onnx.name = "input_y_2", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        return %1 : f32
    }

    func.func @test3(%arg0: f32 {onnx.name = "input_x_3", onnx.type = "encrypted"}, %arg1: f32 {onnx.name = "input_y_3", onnx.type = "clear"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        return %1 : f32
    }

    func.func @test4(%arg0: f32 {onnx.name = "input_x_4"}, %arg1: f32 {onnx.name = "input_y_4"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        return %1 : f32
    }

    func.func @test5(%arg0: f32, %arg1: f32) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        return %1 : f32
    }
}


// CHECK-LABEL: func.func @main_graph
// CHECK: (%arg0: f32 {onnx.dims = [1], onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: f32 {onnx.dims = [1], onnx.name = "input_y", onnx.type = "encrypted"}) -> (f32 {onnx.dims = [1]})