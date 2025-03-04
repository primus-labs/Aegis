// RUN: aegiscompiler --arith-to-secret --canonicalize --cse < %s | FileCheck %s
module {
    // added metadata
    "llvm.metadata"() {param0.name = "input_x", param0.type = "encrypted"} : () -> ()
    "llvm.metadata"() {param1.name = "input_y", param1.type = "encrypted"} : () -> ()
    "llvm.metadata"() {output.name = "output", output.type = "encrypted"} : () -> ()

    func.func @main_graph(%arg0: f32, %arg1:f32 ) -> i1 {
        %result = arith.cmpf "oeq", %arg0, %arg1 : f32
        return %result : i1
    }
}

// CHECK-NOT: arith.cmpf
// CHECK: secret.compare