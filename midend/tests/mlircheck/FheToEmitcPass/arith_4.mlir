// RUN: aegiscompiler --collect-metadata  --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --secret-to-fhe --canonicalize --fhe-to-emitc --canonicalize < %s | FileCheck %s


module {

    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32

        // CHECK-NOT: arith.divf
        // CHECK-NOT: secret.div
        // CHECK-NOT: fhe.lwediv
        // CHECK-NOT: fhe.rlwediv
        // CHECK: emitc.call_opaque "Div"
    }

    func.func @div_1(%arg0: f32 {onnx.name = "input_x1", onnx.type = "encrypted"}, 
                     %arg1: f32 {onnx.name = "input_y1", onnx.type = "clear"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32

        // CHECK-NOT: arith.divf
        // CHECK-NOT: secret.div_plain
        // CHECK-NOT: fhe.lwediv_plain
        // CHECK-NOT: fhe.rlwediv_plain
        // CHECK: emitc.call_opaque "DivPlain"
    }

    func.func @div_2(%arg0: f32 {onnx.name = "input_x2", onnx.type = "clear"}, 
                     %arg1: f32 {onnx.name = "input_y2", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32

        // CHECK-NOT: arith.divf
        // CHECK-NOT: secret.reciprocal
        // CHECK-NOT: secret.mul_plain
        // CHECK-NOT: fhe.lwereciprocal
        // CHECK-NOT: fhe.lwemul_plain
        // CHECK-NOT: fhe.rlwereciprocal
        // CHECK-NOT: fhe.rlwemul_plain
        // CHECK: emitc.call_opaque "Reciprocal"
        // CHECK: emitc.call_opaque "MulPlain"
    }
}