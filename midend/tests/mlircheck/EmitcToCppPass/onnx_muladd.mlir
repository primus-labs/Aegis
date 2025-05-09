// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s


module attributes {fhe.GaloisKeyIndex = dense<[-1, -2, -3, -4, -5, -6, -7]> : vector<7xi32>, llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  emitc.global extern @constant_0 : !emitc.array<1xf32> = dense<2.000000e+00>
  func.func @main_graph(%arg0: !emitc.opaque<"RLWECipher"> {onnx.dims = [8], onnx.name = "input_x"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.dims = [1], onnx.name = "input_y"}) -> (!emitc.opaque<"RLWECipher"> {onnx.dims = [8], onnx.name = "output"}) attributes {llvm.emit_c_interface} {
    %0 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,1,1,1,1,1,1,0)">}> : () -> !emitc.opaque<"PlainVector">
    %1 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,0,0,0,0,0,0,1)">}> : () -> !emitc.opaque<"PlainVector">
    %2 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,1,1,1,1,1,0,1)">}> : () -> !emitc.opaque<"PlainVector">
    %3 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,0,0,0,0,0,1,0)">}> : () -> !emitc.opaque<"PlainVector">
    %4 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,1,1,1,1,0,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %5 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,0,0,0,0,1,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %6 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,1,1,1,0,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %7 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,0,0,0,1,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %8 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,1,1,0,1,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %9 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,0,0,1,0,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %10 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,1,0,1,1,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %11 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,0,1,0,0,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %12 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,0,1,1,1,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %13 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,1,0,0,0,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %14 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,1,1,1,1,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %15 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,0,0,0,0,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %16 = emitc.get_global @constant_0 : !emitc.array<1xf32>
    %17 = emitc.call_opaque "Cast_Stub"(%16) : (!emitc.array<1xf32>) -> !emitc.opaque<"PlainVector">
    %18 = emitc.call_opaque "Native_Load"(%17) : (!emitc.opaque<"PlainVector">) -> !emitc.opaque<"Plain">
    %19 = emitc.call_opaque "MulPlain"(%arg1, %18) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %20 = emitc.call_opaque "Alloc"() : () -> !emitc.opaque<"std::vector<LWECipher>">
    %21 = emitc.call_opaque "Add"(%arg0, %19) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %22 = emitc.call_opaque "MulPlain"(%21, %15) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %23 = emitc.call_opaque "Cast_Stub"(%20) : (!emitc.opaque<"std::vector<LWECipher>">) -> !emitc.opaque<"RLWECipher">
    %24 = emitc.call_opaque "MulPlain"(%23, %14) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %25 = emitc.call_opaque "Add"(%24, %22) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%25, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %26 = emitc.call_opaque "Rotate"(%19) {args = [0 : index, -1 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %27 = emitc.call_opaque "Add"(%arg0, %26) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %28 = emitc.call_opaque "MulPlain"(%27, %13) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %29 = emitc.call_opaque "MulPlain"(%23, %12) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %30 = emitc.call_opaque "Add"(%29, %28) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%30, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %31 = emitc.call_opaque "Rotate"(%19) {args = [0 : index, -2 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %32 = emitc.call_opaque "Add"(%arg0, %31) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %33 = emitc.call_opaque "MulPlain"(%32, %11) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %34 = emitc.call_opaque "MulPlain"(%23, %10) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %35 = emitc.call_opaque "Add"(%34, %33) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%35, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %36 = emitc.call_opaque "Rotate"(%19) {args = [0 : index, -3 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %37 = emitc.call_opaque "Add"(%arg0, %36) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %38 = emitc.call_opaque "MulPlain"(%37, %9) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %39 = emitc.call_opaque "MulPlain"(%23, %8) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %40 = emitc.call_opaque "Add"(%39, %38) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%40, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %41 = emitc.call_opaque "Rotate"(%19) {args = [0 : index, -4 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %42 = emitc.call_opaque "Add"(%arg0, %41) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %43 = emitc.call_opaque "MulPlain"(%42, %7) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %44 = emitc.call_opaque "MulPlain"(%23, %6) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %45 = emitc.call_opaque "Add"(%44, %43) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%45, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %46 = emitc.call_opaque "Rotate"(%19) {args = [0 : index, -5 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %47 = emitc.call_opaque "Add"(%arg0, %46) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %48 = emitc.call_opaque "MulPlain"(%47, %5) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %49 = emitc.call_opaque "MulPlain"(%23, %4) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %50 = emitc.call_opaque "Add"(%49, %48) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%50, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %51 = emitc.call_opaque "Rotate"(%19) {args = [0 : index, -6 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %52 = emitc.call_opaque "Add"(%arg0, %51) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %53 = emitc.call_opaque "MulPlain"(%52, %3) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %54 = emitc.call_opaque "MulPlain"(%23, %2) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %55 = emitc.call_opaque "Add"(%54, %53) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%55, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %56 = emitc.call_opaque "Rotate"(%19) {args = [0 : index, -7 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %57 = emitc.call_opaque "Add"(%arg0, %56) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %58 = emitc.call_opaque "MulPlain"(%57, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %59 = emitc.call_opaque "MulPlain"(%23, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %60 = emitc.call_opaque "Add"(%59, %58) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%60, %23) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    return %23 : !emitc.opaque<"RLWECipher">
  }
}




// CHECK: extern float constant_0[1] = {2.000000000e+00f};
// CHECK: RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
// CHECK:   PlainVector v3 = MakeMultPlain(1,1,1,1,1,1,1,0);
// CHECK:   PlainVector v4 = MakeMultPlain(0,0,0,0,0,0,0,1);
// CHECK:   PlainVector v5 = MakeMultPlain(1,1,1,1,1,1,0,1);
// CHECK:   PlainVector v6 = MakeMultPlain(0,0,0,0,0,0,1,0);
// CHECK:   PlainVector v7 = MakeMultPlain(1,1,1,1,1,0,1,1);
// CHECK:   PlainVector v8 = MakeMultPlain(0,0,0,0,0,1,0,0);
// CHECK:   PlainVector v9 = MakeMultPlain(1,1,1,1,0,1,1,1);
// CHECK:   PlainVector v10 = MakeMultPlain(0,0,0,0,1,0,0,0);
// CHECK:   PlainVector v11 = MakeMultPlain(1,1,1,0,1,1,1,1);
// CHECK:   PlainVector v12 = MakeMultPlain(0,0,0,1,0,0,0,0);
// CHECK:   PlainVector v13 = MakeMultPlain(1,1,0,1,1,1,1,1);
// CHECK:   PlainVector v14 = MakeMultPlain(0,0,1,0,0,0,0,0);
// CHECK:   PlainVector v15 = MakeMultPlain(1,0,1,1,1,1,1,1);
// CHECK:   PlainVector v16 = MakeMultPlain(0,1,0,0,0,0,0,0);
// CHECK:   PlainVector v17 = MakeMultPlain(0,1,1,1,1,1,1,1);
// CHECK:   PlainVector v18 = MakeMultPlain(1,0,0,0,0,0,0,0);
// CHECK:   PlainVector v19 = Cast_Stub(constant_0);
// CHECK:   Plain v20 = Native_Load(v19);
// CHECK:   RLWECipher v21 = MulPlain(v2, v20);
// CHECK:   std::vector<LWECipher> v22 = Alloc();
// CHECK:   RLWECipher v23 = Add(v1, v21);
// CHECK:   RLWECipher v24 = MulPlain(v23, v18);
// CHECK:   RLWECipher v25 = Cast_Stub(v22);
// CHECK:   RLWECipher v26 = MulPlain(v25, v17);
// CHECK:   RLWECipher v27 = Add(v26, v24);
// CHECK:   Copy(v27, v25);
// CHECK:   RLWECipher v28 = Rotate(v21, -1);
// CHECK:   RLWECipher v29 = Add(v1, v28);
// CHECK:   RLWECipher v30 = MulPlain(v29, v16);
// CHECK:   RLWECipher v31 = MulPlain(v25, v15);
// CHECK:   RLWECipher v32 = Add(v31, v30);
// CHECK:   Copy(v32, v25);
// CHECK:   RLWECipher v33 = Rotate(v21, -2);
// CHECK:   RLWECipher v34 = Add(v1, v33);
// CHECK:   RLWECipher v35 = MulPlain(v34, v14);
// CHECK:   RLWECipher v36 = MulPlain(v25, v13);
// CHECK:   RLWECipher v37 = Add(v36, v35);
// CHECK:   Copy(v37, v25);
// CHECK:   RLWECipher v38 = Rotate(v21, -3);
// CHECK:   RLWECipher v39 = Add(v1, v38);
// CHECK:   RLWECipher v40 = MulPlain(v39, v12);
// CHECK:   RLWECipher v41 = MulPlain(v25, v11);
// CHECK:   RLWECipher v42 = Add(v41, v40);
// CHECK:   Copy(v42, v25);
// CHECK:   RLWECipher v43 = Rotate(v21, -4);
// CHECK:   RLWECipher v44 = Add(v1, v43);
// CHECK:   RLWECipher v45 = MulPlain(v44, v10);
// CHECK:   RLWECipher v46 = MulPlain(v25, v9);
// CHECK:   RLWECipher v47 = Add(v46, v45);
// CHECK:   Copy(v47, v25);
// CHECK:   RLWECipher v48 = Rotate(v21, -5);
// CHECK:   RLWECipher v49 = Add(v1, v48);
// CHECK:   RLWECipher v50 = MulPlain(v49, v8);
// CHECK:   RLWECipher v51 = MulPlain(v25, v7);
// CHECK:   RLWECipher v52 = Add(v51, v50);
// CHECK:   Copy(v52, v25);
// CHECK:   RLWECipher v53 = Rotate(v21, -6);
// CHECK:   RLWECipher v54 = Add(v1, v53);
// CHECK:   RLWECipher v55 = MulPlain(v54, v6);
// CHECK:   RLWECipher v56 = MulPlain(v25, v5);
// CHECK:   RLWECipher v57 = Add(v56, v55);
// CHECK:   Copy(v57, v25);
// CHECK:   RLWECipher v58 = Rotate(v21, -7);
// CHECK:   RLWECipher v59 = Add(v1, v58);
// CHECK:   RLWECipher v60 = MulPlain(v59, v4);
// CHECK:   RLWECipher v61 = MulPlain(v25, v3);
// CHECK:   RLWECipher v62 = Add(v61, v60);
// CHECK:   Copy(v62, v25);
// CHECK:   return v25;
// CHECK: }