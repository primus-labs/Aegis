// RUN: aegiscompiler --arith-to-secret  < %s | FileCheck %s
module {
    // added metadata
    "llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    "llvm.metadata"() {param1.name = "input_y", param1.type = "encrypted"} : () -> ()
    "llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32, %arg1:f32 ) -> f32 {
        %5 = arith.subf %arg0, %arg1 :  f32
        return %5 : f32
    }
}

// CHECK-NOT: arith.subf
// CHECK: secret.sub