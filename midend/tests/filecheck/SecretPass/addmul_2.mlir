// RUN: aegiscompiler --arith-to-secret --canonicalize --cse < %s | FileCheck %s
module {
    // added metadata
    //"llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    //"llvm.metadata"() {param1.name = "input_y", param1.type = "encrypted"} : () -> ()
    //"llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32, %arg1:f32 ) -> f32 {
        %3 = arith.mulf %arg0, %arg1 : f32
        %4 = arith.addf %arg0, %arg1 : f32
        %5 = arith.mulf %3, %4 :  f32
        return %5 : f32
    }
}

// CHECK-NOT: arith.mulf
// CHECK-NOT: arith.addf
// CHECK: secret.mul
// CHECK: secret.add