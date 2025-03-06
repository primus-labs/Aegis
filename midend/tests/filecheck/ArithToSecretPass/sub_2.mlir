// RUN: aegiscompiler --collect-metadata --arith-to-secret < %s | FileCheck %s
module {
    // added metadata
    // "llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    // "llvm.metadata"() {param1.name = "input_y", param1.type = "clear"} : () -> ()
    // "llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %1 = arith.subf %arg0, %arg1 :  f32
        %cst = arith.constant 0.0 : f32
        %2 = arith.subf %1, %cst : f32
        %3 = arith.subf %cst, %2 : f32
        %4 = arith.subf %2, %3 : f32
        return %4 : f32
    }
}

// CHECK-NOT: arith.subf
// CHECK: secret.sub
// CHECK: secret.neg