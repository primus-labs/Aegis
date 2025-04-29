// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --auto-bootstrap --canonicalize --cse < %s | FileCheck %s

module {

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.mulf %arg0, %arg1 :  f32
        %2 = arith.mulf %1, %arg0 : f32
        %3 = arith.mulf %1, %2 : f32
        %4 = arith.mulf %2, %3 : f32
        %5 = arith.mulf %3, %4 : f32
        %6 = arith.mulf %4, %5 : f32
        %7 = arith.mulf %5, %6 : f32
        %8 = arith.mulf %6, %7 : f32
        %9 = arith.mulf %7, %8 : f32
        return %9 : f32
    }
}

//CHECK:fhe.bootstrap
//CHECK:fhe.bootstrap
