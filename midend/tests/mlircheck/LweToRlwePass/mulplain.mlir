// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse  --lwe-to-rlwe --canonicalize --cse  < %s | FileCheck %s

module {

    // -----
    // CHECK-LABEL: @mul
    func.func @mul(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %5 = arith.mulf %arg0, %arg1 :  f32
        return %5 : f32
    }
    // CHECK-NOT: arith.mulf
    // CHECK-NOT: secret.mul_plain
    // CHECK-NOT: fhe.lwemulplain
    // CHECK: (%arg0: !fhe.rlwecipher<1 x f32> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> !fhe.rlwecipher<1 x f32>
    // CHECK: fhe.rlwemulplain
}