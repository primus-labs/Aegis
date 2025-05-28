// RUN: aegiscompiler --collect-metadata --unroll-loop-and-memory-opt --canonicalize --cse --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --cse --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse --lower-multi-dim-load --canonicalize --cse --batching --canonicalize --cse --loadstore-to-copy --canonicalize --cse --lwe-to-rlwe --canonicalize --cse --fhe-to-emitc --canonicalize --cse  < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "add"} {
  func.func @main_graph(%arg0: memref<3x2xf32> {onnx.name = "X1"}, %arg1: memref<3x2xf32> {onnx.name = "X2"}) -> (memref<3x2xf32> {onnx.name = "Y"}) attributes {llvm.emit_c_interface} {
    %alloc = memref.alloc() {alignment = 16 : i64} : memref<3x2xf32>
    affine.for %arg2 = 0 to 3 {
      affine.for %arg3 = 0 to 2 {
        %0 = affine.load %arg0[%arg2, %arg3] : memref<3x2xf32>
        %1 = affine.load %arg1[%arg2, %arg3] : memref<3x2xf32>
        %2 = arith.addf %0, %1 : f32
        affine.store %2, %alloc[%arg2, %arg3] : memref<3x2xf32>
      }
    }
    return %alloc : memref<3x2xf32>
  }
}


//CHECK: %8 = emitc.call_opaque "AllocArray"() {args = [3, 16]} : () -> !emitc.opaque<"std::vector<RLWECipher>"> 
//CHECK: %9 = emitc.call_opaque "Vload"(%arg0, %5) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %10 = emitc.call_opaque "Vload"(%arg1, %5) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %11 = emitc.call_opaque "Add"(%9, %10) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: %12 = emitc.call_opaque "MulPlain"(%11, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %13 = emitc.call_opaque "Vload"(%8, %5) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %14 = emitc.call_opaque "MulPlain"(%13, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %15 = emitc.call_opaque "Add"(%14, %12) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: emitc.call_opaque "Vstore"(%15, %8, %5) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"std::vector<RLWECipher>">, index) -> ()
//CHECK: %16 = emitc.call_opaque "Vload"(%arg0, %5) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %17 = emitc.call_opaque "Vload"(%arg1, %5) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %18 = emitc.call_opaque "Add"(%16, %17) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: %19 = emitc.call_opaque "MulPlain"(%18, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %20 = emitc.call_opaque "Vload"(%8, %5) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %21 = emitc.call_opaque "MulPlain"(%20, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %22 = emitc.call_opaque "Add"(%21, %19) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: emitc.call_opaque "Vstore"(%22, %8, %5) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"std::vector<RLWECipher>">, index) -> ()
//CHECK: %23 = emitc.call_opaque "Vload"(%arg0, %6) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %24 = emitc.call_opaque "Vload"(%arg1, %6) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %25 = emitc.call_opaque "Add"(%23, %24) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: %26 = emitc.call_opaque "MulPlain"(%25, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %27 = emitc.call_opaque "Vload"(%8, %6) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %28 = emitc.call_opaque "MulPlain"(%27, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %29 = emitc.call_opaque "Add"(%28, %26) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: emitc.call_opaque "Vstore"(%29, %8, %6) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"std::vector<RLWECipher>">, index) -> ()
//CHECK: %30 = emitc.call_opaque "Vload"(%arg0, %6) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %31 = emitc.call_opaque "Vload"(%arg1, %6) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %32 = emitc.call_opaque "Add"(%30, %31) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: %33 = emitc.call_opaque "MulPlain"(%32, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %34 = emitc.call_opaque "Vload"(%8, %6) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %35 = emitc.call_opaque "MulPlain"(%34, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %36 = emitc.call_opaque "Add"(%35, %33) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: emitc.call_opaque "Vstore"(%36, %8, %6) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"std::vector<RLWECipher>">, index) -> ()
//CHECK: %37 = emitc.call_opaque "Vload"(%arg0, %7) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %38 = emitc.call_opaque "Vload"(%arg1, %7) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %39 = emitc.call_opaque "Add"(%37, %38) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: %40 = emitc.call_opaque "MulPlain"(%39, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %41 = emitc.call_opaque "Vload"(%8, %7) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %42 = emitc.call_opaque "MulPlain"(%41, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %43 = emitc.call_opaque "Add"(%42, %40) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: emitc.call_opaque "Vstore"(%43, %8, %7) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"std::vector<RLWECipher>">, index) -> ()
//CHECK: %44 = emitc.call_opaque "Vload"(%arg0, %7) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %45 = emitc.call_opaque "Vload"(%arg1, %7) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %46 = emitc.call_opaque "Add"(%44, %45) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: %47 = emitc.call_opaque "MulPlain"(%46, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %48 = emitc.call_opaque "Vload"(%8, %7) : (!emitc.opaque<"std::vector<RLWECipher>">, index) -> !emitc.opaque<"RLWECipher">
//CHECK: %49 = emitc.call_opaque "MulPlain"(%48, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
//CHECK: %50 = emitc.call_opaque "Add"(%49, %47) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
//CHECK: emitc.call_opaque "Vstore"(%50, %8, %7) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"std::vector<RLWECipher>">, index) -> ()