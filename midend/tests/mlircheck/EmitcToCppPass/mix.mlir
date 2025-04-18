// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  emitc.global extern @constant_0 : !emitc.array<1xf32> = dense<2.000000e+00>
  func.func @main_graph(%arg0: !emitc.opaque<"std::vector<LWECipher>"> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"std::vector<LWECipher>"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"std::vector<LWECipher>"> attributes {llvm.emit_c_interface} {
    %0 = "emitc.constant"() <{value = #emitc.opaque<"Plain(0.000000)">}> : () -> !emitc.opaque<"Plain">
    %1 = "emitc.constant"() <{value = #emitc.opaque<"Plain(1.000000)">}> : () -> !emitc.opaque<"Plain">
    %2 = emitc.call_opaque "Cast_Stub"(%1) : (!emitc.opaque<"Plain">) -> index
    %3 = emitc.call_opaque "Cast_Stub"(%0) : (!emitc.opaque<"Plain">) -> index
    %4 = emitc.get_global @constant_0 : !emitc.array<1xf32>
    %5 = emitc.call_opaque "Cast_Stub"(%4) : (!emitc.array<1xf32>) -> !emitc.opaque<"PlainVector">
    %6 = emitc.call_opaque "Load"(%arg1, %3) : (!emitc.opaque<"std::vector<LWECipher>">, index) -> !emitc.opaque<"LWECipher">
    %7 = emitc.call_opaque "Native_Load"(%5) : (!emitc.opaque<"PlainVector">) -> !emitc.opaque<"Plain">
    %8 = emitc.call_opaque "MulPlain"(%6, %7) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"LWECipher">
    %9 = emitc.call_opaque "Alloc"() : () -> !emitc.opaque<"std::vector<LWECipher>">
    %10 = emitc.call_opaque "Load"(%arg0, %3) : (!emitc.opaque<"std::vector<LWECipher>">, index) -> !emitc.opaque<"LWECipher">
    %11 = emitc.call_opaque "Add"(%10, %8) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"LWECipher">) -> !emitc.opaque<"LWECipher">
    %12 = emitc.call_opaque "Store"(%9, %11, %3) : (!emitc.opaque<"std::vector<LWECipher>">, !emitc.opaque<"LWECipher">, index) -> !emitc.opaque<"std::vector<LWECipher>">
    %13 = emitc.call_opaque "Load"(%arg0, %2) : (!emitc.opaque<"std::vector<LWECipher>">, index) -> !emitc.opaque<"LWECipher">
    %14 = emitc.call_opaque "Add"(%13, %8) : (!emitc.opaque<"LWECipher">, !emitc.opaque<"LWECipher">) -> !emitc.opaque<"LWECipher">
    %15 = emitc.call_opaque "Store"(%9, %14, %2) : (!emitc.opaque<"std::vector<LWECipher>">, !emitc.opaque<"LWECipher">, index) -> !emitc.opaque<"std::vector<LWECipher>">
    return %9 : !emitc.opaque<"std::vector<LWECipher>">
  }
}



// CHECK: extern float constant_0[1] = {2.000000000e+00f};
// CHECK:std::vector<LWECipher> main_graph(std::vector<LWECipher> v1, std::vector<LWECipher> v2) {
// CHECK:  Plain v3 = Plain(0.000000);
// CHECK:  Plain v4 = Plain(1.000000);
// CHECK:  size_t v5 = Cast_Stub(v4);
// CHECK:  size_t v6 = Cast_Stub(v3);
// CHECK:  PlainVector v7 = Cast_Stub(constant_0);
// CHECK:  LWECipher v8 = Load(v2, v6);
// CHECK:  Plain v9 = Native_Load(v7);
// CHECK:  LWECipher v10 = MulPlain(v8, v9);
// CHECK:  std::vector<LWECipher> v11 = Alloc();
// CHECK:  LWECipher v12 = Load(v1, v6);
// CHECK:  LWECipher v13 = Add(v12, v10);
// CHECK:  std::vector<LWECipher> v14 = Store(v11, v13, v6);
// CHECK:  LWECipher v15 = Load(v1, v5);
// CHECK:  LWECipher v16 = Add(v15, v10);
// CHECK:  std::vector<LWECipher> v17 = Store(v11, v16, v5);
// CHECK:  return v11;
// CHECK:}

