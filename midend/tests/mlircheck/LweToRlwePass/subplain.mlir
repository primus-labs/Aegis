// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse  --lwe-to-rlwe --canonicalize --cse  < %s | FileCheck %s

module {

    // -----
    // CHECK-LABEL: @sub
    func.func @sub(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %3 = arith.subf %arg0, %arg1 : f32
        return %3 : f32
    }
}


// CHECK-NOT: arith.subf
// CHECK-NOT: secret.sub_plain
// CHECK-NOT: fhe.lwesub_plain
// CHECK: (%arg0: !fhe.rlwecipher<1 x f32> {onnx.dims = [1], onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: f32 {onnx.dims = [1], onnx.name = "input_y", onnx.type = "clear"}) -> (!fhe.rlwecipher<1 x f32> {onnx.dims = [1]})
// CHECK: fhe.rlwesub_plain
