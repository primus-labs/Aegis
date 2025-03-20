// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  emitc.global extern @constant_0 : !emitc.array<1xf32> = dense<2.000000e+00>
  
  func.func @main_graph(%arg0: !emitc.opaque<"LWECipher"> {onnx.name = "input_x", onnx.type = "encrypted"}) -> !emitc.opaque<"LWECipher"> attributes {llvm.emit_c_interface} {
    %0 = emitc.get_global @constant_0 : !emitc.array<1xf32>
    // %1 = fhe.cast(%0) : (!emitc.array<f32>) -> !emitc.opaque<"std::vector<Plain>">
    %1 = emitc.call_opaque "Cast"(%0) : (!emitc.array<1xf32>) -> !emitc.opaque<"std::vector<Plain>">
    %2 = emitc.call_opaque "Native_Load"(%1) : (!emitc.opaque<"std::vector<Plain>">) -> !emitc.opaque<"Plain">
    %3 = emitc.call_opaque "MulPlain"(%arg0, %2) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"LWECipher">
    return %3 : !emitc.opaque<"LWECipher">
  }
}


// CHECK: extern float constant_0[1] = {2.000000000e+00f};
// CHECK: LWECipher main_graph(LWECipher v1) {
// CHECK: std::vector<Plain> v2 = Cast(constant_0);
// CHECK: Plain v3 = Native_Load(v2);
// CHECK: LWECipher v4 = MulPlain(v1, v3);
// CHECK: return v4;
// CHECK: }
