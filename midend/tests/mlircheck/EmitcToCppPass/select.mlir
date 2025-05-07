// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module {
  func.func @main_graph(%arg0: !emitc.opaque<"RLWECipher"> {onnx.dims = [1], onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.dims = [1], onnx.name = "input_y", onnx.type = "encrypted"}) -> (!emitc.opaque<"RLWECipher"> {onnx.dims = [1]}) {
    %0 = emitc.call_opaque "Cmp_lt"(%arg0, %arg0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %1 = emitc.call_opaque "Select"(%0, %arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    return %1 : !emitc.opaque<"RLWECipher">
  }
}


//CHECK: RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
//CHECK:   RLWECipher v3 = Cmp_lt(v1, v1);
//CHECK:   RLWECipher v4 = Select(v3, v1, v2);
//CHECK:   return v4;
//CHECK: }