// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub --insert-emitc-preamble --cse < %s | FileCheck %s

module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %5 = arith.addf %arg0, %arg1 :  f32
        %6 = arith.mulf %arg0, %5 :  f32
        return %6 : f32
    }
}



// CHECK: emitc.call_opaque "AddPlain"
// CHECK: emitc.call_opaque "Mul"

