// RUN: aegiscompiler --arith-to-secret  < %s | FileCheck %s
module {
    // added metadata
    "llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    "llvm.metadata"() {param1.name = "input_y", param1.type = "clear"} : () -> ()
    "llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32, %arg1:f32 ) -> f32 {
        %1 = arith.mulf %arg0, %arg1 :  f32
        %cst = arith.constant 1.0 : f32
        %2 = arith.mulf %1, %cst : f32
        %3 = arith.mulf %cst, %2 : f32
        return %3 : f32
    }
}

// CHECK-NOT: arith.mulf
// CHECK: secret.mul