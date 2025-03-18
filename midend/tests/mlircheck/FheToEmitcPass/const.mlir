// RUN: aegiscompiler --collect-metadata  --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fhe-to-emitc --canonicalize --cse < %s | FileCheck %s

module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %c0 = arith.constant 1.0 : f32
        %3 = arith.subf %arg0, %c0 : f32

        %c1 = arith.constant 2.0 : f32
        %4 = arith.addf %arg1, %c1 : f32

        %5 = arith.mulf %3, %4 :  f32
        return %5 : f32
    }
}


// CHECK-NOT: arith.subf
// CHECK-NOT: arith.addf
// CHECK-NOT: arith.mulf
// CHECK-NOT: secret.subplain
// CHECK-NOT: secret.addplain
// CHECK-NOT: secret.mul
// CHECK-NOT: fhe.subplain
// CHECK-NOT: fhe.addplain
// CHECK-NOT: fhe.mul
// CHECK: emitc.constant
// CHECK: emitc.call_opaque "SubPlain"
// CHECK: emitc.call_opaque "AddPlain"
// CHECK: emitc.call_opaque "Mul"
