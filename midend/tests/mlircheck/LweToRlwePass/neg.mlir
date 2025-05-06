// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse  --lwe-to-rlwe --canonicalize --cse  < %s | FileCheck %s

module {

    // -----
    // CHECK-LABEL: @sub
    func.func @sub(%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %3 = arith.subf %arg0, %arg1 : f32
        return %3 : f32
    }
}


// CHECK-NOT: arith.subf
// CHECK-NOT: secret.neg
// CHECK-NOT: secret.add_plain
// CHECK-NOT: fhe.lweneg
// CHECK-NOT: fhe.lweadd_plain
// CHECK: (%arg0: f32 {onnx.dims = [1], onnx.name = "input_x", onnx.type = "clear"}, %arg1: !fhe.rlwecipher<1 x f32> {onnx.dims = [1], onnx.name = "input_y", onnx.type = "encrypted"}) -> (!fhe.rlwecipher<1 x f32> {onnx.dims = [1]})
// CHECK: fhe.rlweneg
// CHECK: fhe.rlweadd_plain
