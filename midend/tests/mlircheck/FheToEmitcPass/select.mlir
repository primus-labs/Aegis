// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --batching --canonicalize --cse --lwe-to-rlwe --canonicalize --cse  --fhe-to-emitc --canonicalize --cse < %s | FileCheck %s

module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %cond = arith.cmpf "olt", %arg0, %arg1 : f32
        %result = arith.select %cond, %arg0, %arg1 : f32
        return %result : f32
    }
}


//CHECK: emitc.call_opaque "Cmp_lt"
//CHECK: emitc.call_opaque "Select"
