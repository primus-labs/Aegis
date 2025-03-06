// RUN: aegiscompiler --func-to-secret --canonicalize --cse < %s | FileCheck %s
module {
    //"llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    //"llvm.metadata"() {param1.name = "input_y", param1.type = "encrypted"} : () -> ()
    //"llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()
    func.func @main_graph(%arg0: f32, %arg1: f32) -> f32 {
        %0 = "secret.cast"(%arg0) : (f32) -> !secret.secret<f32>
        %1 = "secret.cast"(%arg1) : (f32) -> !secret.secret<f32>
        %2 = "secret.add"(%0, %1) : (!secret.secret<f32>, !secret.secret<f32>) -> !secret.secret<f32>
        %3 = "secret.cast"(%2) : (!secret.secret<f32>) -> f32
        return %3 : f32
    }
}


// CHECK-LABEL: func.func @main_graph
// CHECK: (%arg0: !secret.secret<f32>, %arg1: !secret.secret<f32>) -> !secret.secret<f32>