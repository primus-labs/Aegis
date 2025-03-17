// RUN: aegiscompiler --collect-metadata  --arith-to-secret --canonicalize --cse  < %s | FileCheck %s

module {

func.func @arith_1(%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %3 = arith.subf %arg0, %arg1 : f32
        %4 = arith.addf %arg0, %arg1 : f32
        %5 = arith.mulf %3, %4 :  f32
        return %5 : f32

        // CHECK-NOT: arith.subf
        // CHECK-NOT: arith.addf
        // CHECK-NOT: arith.mulf
        // CHECK: secret.neg
        // CHECK: secret.add_plain
        // CHECK: secret.add_plain
        // CHECK: secret.mul
    }
}
