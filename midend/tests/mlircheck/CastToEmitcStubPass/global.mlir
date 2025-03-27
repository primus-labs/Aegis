// RUN: aegiscompiler --collect-metadata --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --fhe-to-emitc --canonicalize --cse --cast-to-emitc-stub < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  memref.global @constant_0 : memref<f32> = dense<2.000000e+00>

  func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}) -> f32 attributes {llvm.emit_c_interface} {
    %0 = memref.get_global @constant_0 : memref<f32>
    %1 = memref.load %0[] : memref<f32>
    %2 = arith.mulf %arg0, %1 : f32
    return %2 : f32
  }
}


// CHECK-NOT: memref.global
// CHECK-NOT: memref.get_global
// CHECK-NOT: memref.load
// CHECK-NOT: arith.mulf
// CHECK-NOT: secret.addplain
// CHECK-NOT: fhe.lweadd_plain
// CHECK: emitc.global
// CHECK: emitc.get_global
// CHECK: emitc.call_opaque "Cast_Stub"
// CHECK: emitc.call_opaque "Native_Load"
// CHECK: emitc.call_opaque "MulPlain"