// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse  --lwe-to-rlwe --canonicalize --cse  < %s | FileCheck %s

module {

    // -----
    // CHECK-LABEL: @add
    func.func @add(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.addf %arg0, %arg1 :  f32
        return %5 : f32
    }
    // CHECK-NOT: arith.addf
    // CHECK-NOT: secret.add
    // CHECK-NOT: fhe.lweadd
    // CHECK: (%arg0: !fhe.rlwecipher<1 x f32> {onnx.dims = [1], onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !fhe.rlwecipher<1 x f32> {onnx.dims = [1], onnx.name = "input_y", onnx.type = "encrypted"}) -> (!fhe.rlwecipher<1 x f32> {onnx.dims = [1]})
    // CHECK: fhe.rlweadd
}