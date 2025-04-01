// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fold-arith-chain --canonicalize --cse < %s | FileCheck %s


module {
    // -----
    func.func @add(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        %2 = arith.addf %1, %arg0 : f32
        return %2 : f32
    }

    // CHECK-NOT: arith.addf
    // CHECK-NOT: secret.add
    // CHECK: fhe.lweadd(%arg0, %arg0, %arg1)


    // -----
    func.func @add_2(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                     %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        %2 = arith.addf %1, %arg0 : f32
        %3 = arith.addf %2, %arg1 : f32
        return %3 : f32
    }

    // CHECK-NOT: arith.addf
    // CHECK-NOT: secret.add
    // CHECK: fhe.lweadd(%arg1, %arg0, %arg0, %arg1)


    // -----
    func.func @add_plain(%arg0: f32 {onnx.name = "input2_x", onnx.type = "encrypted"}, 
                         %arg1: f32 {onnx.name = "input2_y", onnx.type = "clear"}) -> f32 {
        %1 = arith.addf %arg0, %arg1 :  f32
        %2 = arith.addf %1, %arg0 : f32
        return %2 : f32
    }
}