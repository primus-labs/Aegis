// RUN: aegiscompiler --collect-metadata --arith-to-secret --func-to-secret --canonicalize --cse < %s | FileCheck %s
module {
    // added metadata
    // "llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    // "llvm.metadata"() {param1.name = "input_y", param1.type = "clear"} : () -> ()
    // "llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1:f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        %cst = arith.constant 0.0 : f32
        %2 = arith.addf %1, %cst : f32
        %3 = arith.addf %cst, %2 : f32
        return %3 : f32
    }

}


// CHECK-LABEL: func.func @main_graph
// CHECK: (%arg0: !secret.secret<f32> {onnx.dims = [1], onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: f32 {onnx.dims = [1], onnx.name = "input_y", onnx.type = "clear"}) -> (!secret.secret<f32> {onnx.dims = [1]})
// CHECK: secret.add_plain