// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module {
  func.func @main_graph(%arg0: !emitc.opaque<"RLWECipher"> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                        %arg1: !emitc.opaque<"RLWECipher"> {onnx.name = "input_y", onnx.type = "encrypted"}) 
                        -> !emitc.opaque<"RLWECipher"> {
    %0 = emitc.call_opaque "Sub"(%arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %1 = emitc.call_opaque "Add"(%arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %2 = emitc.call_opaque "Mul"(%0, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    return %2 : !emitc.opaque<"RLWECipher">
  }
}


// CHECK-NOT: emitc.call_opaque
// CHECK: RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
// CHECK: RLWECipher v3 = Sub(v1, v2);
// CHECK: RLWECipher v4 = Add(v1, v2);
// CHECK: RLWECipher v5 = Mul(v3, v4);
// CHECK: return v5;
// CHECK: }



