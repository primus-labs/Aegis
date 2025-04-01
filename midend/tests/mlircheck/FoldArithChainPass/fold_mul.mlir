// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fold-arith-chain --canonicalize --cse < %s | FileCheck %s


module {
    // -----
    func.func @mul(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.mulf %arg0, %arg1 :  f32
        %2 = arith.mulf %1, %arg0 : f32
        return %2 : f32
    }

    // CHECK-NOT: arith.mulf
    // CHECK-NOT: secret.mul
    // CHECK: fhe.lwemul(%arg0, %arg0, %arg1)


    // -----
    func.func @mul_2(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                     %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.mulf %arg0, %arg1 :  f32
        %2 = arith.mulf %1, %arg0 : f32
        %3 = arith.mulf %2, %arg1 : f32
        return %3 : f32
    }

    // CHECK-NOT: arith.mulf
    // CHECK-NOT: secret.mul
    // CHECK: fhe.lwemul(%arg1, %arg0, %arg0, %arg1)


    // -----
    func.func @mul_plain(%arg0: f32 {onnx.name = "input2_x", onnx.type = "encrypted"}, 
                         %arg1: f32 {onnx.name = "input2_y", onnx.type = "clear"}) -> f32 {
        %1 = arith.mulf %arg0, %arg1 :  f32
        %2 = arith.mulf %1, %arg0 : f32
        return %2 : f32
    }
}