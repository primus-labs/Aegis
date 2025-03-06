// RUN: aegiscompiler --collect-metadata --arith-to-secret --func-to-secret --canonicalize --cse < %s | FileCheck %s
module {
    // added metadata
    // "llvm.metadata"() {param0.name = "input_x", param0.type = "clear"} : () -> ()
    // "llvm.metadata"() {param1.name = "input_y", param1.type = "encrypted"} : () -> ()
    // "llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, %arg1:f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        %cst = arith.constant 0.0 : f32
        %2 = arith.addf %1, %cst : f32
        %3 = arith.addf %cst, %2 : f32
        return %3 : f32
    }
}


// CHECK-LABEL: func.func @main_graph
// CHECK: (%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, %arg1: !secret.secret<f32> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !secret.secret<f32>
// CHECK: secret.add_plain