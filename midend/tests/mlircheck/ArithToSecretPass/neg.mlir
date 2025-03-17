// RUN: aegiscompiler --collect-metadata  --arith-to-secret --canonicalize --cse  < %s | FileCheck %s

module {

func.func @arith_1(%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %3 = arith.subf %arg0, %arg1 : f32
        return %3 : f32

        // CHECK-NOT: arith.subf
        // CHECK: secret.neg
        // CHECK: secret.add_plain
    }
}
