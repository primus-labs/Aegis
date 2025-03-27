// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse  --lwe-to-rlwe --canonicalize --cse  < %s | FileCheck %s

module {

    // -----
    // CHECK-LABEL: @add_en_vs_clr 
    func.func @add_en_vs_clr(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                             %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %5 = arith.addf %arg0, %arg1 :  f32
        return %5 : f32
    }
    // CHECK-NOT: arith.addf
    // CHECK-NOT: secret.addplain
    // CHECK-NOT: fhe.lweaddplain
    // CHECK: (%arg0: !fhe.rlwecipher<1 x f32> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> !fhe.rlwecipher<1 x f32>
    // CHECK: fhe.rlweaddplain
}