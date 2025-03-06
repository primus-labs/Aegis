// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse < %s | FileCheck %s
module {
    // added metadata
    // "llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    // "llvm.metadata"() {param1.name = "input_y", param1.type = "encrypted"} : () -> ()
    // "llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %cond = arith.cmpf "olt", %arg0, %arg1 : f32
        %result = arith.select %cond, %arg0, %arg1 : f32
        return %result : f32
    }
}

// CHECK-NOT: arith.cmpf
// CHECK-NOT: arith.select
// CHECK: secret.compare
// CHECK: secret.select