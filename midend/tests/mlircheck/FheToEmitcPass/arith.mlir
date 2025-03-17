// RUN: aegiscompiler --collect-metadata  --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --secret-to-fhe --canonicalize --fhe-to-emitc --canonicalize < %s | FileCheck %s

module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %3 = arith.subf %arg0, %arg1 : f32
        %4 = arith.addf %arg0, %arg1 : f32
        %5 = arith.mulf %3, %4 :  f32
        return %5 : f32
    }
    
}


// CHECK-NOT: arith.subf
// CHECK-NOT: arith.addf
// CHECK-NOT: arith.mulf
// CHECK-NOT: secret.sub
// CHECK-NOT: secret.add
// CHECK-NOT: secret.mul
// CHECK-NOT: fhe.sub
// CHECK-NOT: fhe.add
// CHECK-NOT: fhe.mul
// CHECK: emitc.call_opaque "Sub"
// CHECK: emitc.call_opaque "Add"
// CHECK: emitc.call_opaque "Mul"