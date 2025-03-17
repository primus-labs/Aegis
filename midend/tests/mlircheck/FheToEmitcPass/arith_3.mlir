// RUN: aegiscompiler --collect-metadata  --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --secret-to-fhe --canonicalize --fhe-to-emitc --canonicalize < %s | FileCheck %s

module {

    func.func @arith_1(%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, 
                   %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %3 = arith.subf %arg0, %arg1 : f32
        %4 = arith.addf %arg0, %arg1 : f32
        %5 = arith.mulf %3, %4 :  f32
        return %5 : f32

        // CHECK-NOT: arith.subf
        // CHECK-NOT: arith.addf
        // CHECK-NOT: arith.mulf
        // CHECK-NOT: secret.sub
        // CHECK-NOT: secret.add
        // CHECK-NOT: secret.mul
        // CHECK-NOT: fhe.sub
        // CHECK-NOT: fhe.add
        // CHECK-NOT: fhe.mul
        // CHECK: emitc.call_opaque "Neg"
        // CHECK: emitc.call_opaque "AddPlain"
        // CHECK: emitc.call_opaque "Mul"
    }

    func.func @arith_2(%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, 
                       %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
            %3 = arith.subf %arg0, %arg1 : f32
            %4 = arith.addf %arg0, %arg1 : f32
            %5 = arith.mulf %3, %4 :  f32
            %6 = arith.mulf %5, %arg0 :  f32
            return %6 : f32

            // CHECK-NOT: arith.subf
            // CHECK-NOT: arith.addf
            // CHECK-NOT: arith.mulf
            // CHECK-NOT: secret.sub
            // CHECK-NOT: secret.add
            // CHECK-NOT: secret.mul
            // CHECK-NOT: fhe.sub
            // CHECK-NOT: fhe.add
            // CHECK-NOT: fhe.mul
            // CHECK: emitc.call_opaque "Neg"
            // CHECK: emitc.call_opaque "AddPlain"
            // CHECK: emitc.call_opaque "Mul"
            // CHECK: emitc.call_opaque "MulPlain"
    }
}
