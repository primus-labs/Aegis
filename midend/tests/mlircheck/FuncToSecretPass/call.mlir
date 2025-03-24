// RUN: aegiscompiler --collect-metadata --arith-to-secret --func-to-secret --canonicalize --cse < %s | FileCheck %s


module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {

    func.func @add(%a : f32, %b : f32) -> f32 {
        %sum = arith.addf %a, %b : f32
        func.return %sum : f32
    }

  func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                        %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %result = func.call @add(%arg0, %arg1) : (f32, f32) -> f32
        func.return %result : f32
    }

}


// CHECK: secret.add
// CHECK: secret.call