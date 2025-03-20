// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  //memref.global @constant_0 : memref<f32> = dense<2.000000e+00>

  func.func @main_graph(%arg0: !emitc.opaque<"std::vector<LWECipher>"> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"std::vector<LWECipher>"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"std::vector<LWECipher>"> {
    emitc.call_opaque "Copy"(%arg0, %arg1) : (!emitc.opaque<"std::vector<LWECipher>">, !emitc.opaque<"std::vector<LWECipher>">) -> ()
    return %arg1 : !emitc.opaque<"std::vector<LWECipher>">
  }
}


// CHECK: std::vector<LWECipher> main_graph(std::vector<LWECipher> v1, std::vector<LWECipher> v2) {
// CHECK-NEXT: Copy(v1, v2);
// CHECK-NEXT: return v2;
// CHECK-NEXT: }
