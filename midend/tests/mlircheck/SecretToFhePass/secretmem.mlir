// RUN: aegiscompiler --collect-metadata  --unroll-loop-and-memory-opt --affine-simplify-structures --lower-affine --arith-to-secret --canonicalize --func-to-secret --canonicalize --cse --memref-to-secret --canonicalize --cse --secret-to-fhe --canonicalize --cse < %s | FileCheck %s

module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "muladd_model"} {
  memref.global @constant_0 : memref<f32> = dense<2.000000e+00>
  func.func @main_graph(%arg0: !secret.secret_vector<8 x f32> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                        %arg1: !secret.secret<f32> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !secret.secret_vector<8 x f32> attributes {llvm.emit_c_interface} {
    %c0 = arith.constant 0 : index
    %0 = memref.get_global @constant_0 : memref<f32>
    %1 = "secret.alloc"() : () -> !secret.secret_vector<8 x f32>
    %3 = memref.load %0[] : memref<f32>
    %4 = "secret.mul_plain"(%arg1, %3) : (!secret.secret<f32>, f32) -> !secret.secret<f32>
    "secret.store"(%4, %1, %c0) : (!secret.secret<f32>, !secret.secret_vector<8 x f32>, index) -> ()
    return %1 : !secret.secret_vector<8 x f32>

    // CHECK-NOT: affine.store
    // CHECK-NOT: affine.load
    // CHECK-NOT: secret.load
    // CHECK-NOT: secret.store
    // CHECK: fhe.alloc
    // CHECK: memref.load
    // CHECK: fhe.lwemulplain
    // CHECK: fhe.store
  }


    func.func @mem2(%arg0: !secret.secret_vector<8 x f32> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                    %arg1: !secret.secret_vector<1 x f32> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !secret.secret_vector<8 x f32> attributes {llvm.emit_c_interface} {
        %c0 = arith.constant 0 : index
        %0 = memref.get_global @constant_0 : memref<f32>
        %1 = "secret.alloc"() : () -> !secret.secret_vector<8 x f32>
        %2 = "secret.load"(%arg1, %c0) : (!secret.secret_vector<1 x f32>, index) -> !secret.secret<f32>
        %3 = memref.load %0[] : memref<f32>
        %4 = "secret.mul_plain"(%2, %3) : (!secret.secret<f32>, f32) -> !secret.secret<f32>
        "secret.store"(%4, %1, %c0) : (!secret.secret<f32>, !secret.secret_vector<8 x f32>, index) -> ()
        return %1 : !secret.secret_vector<8 x f32>

        // CHECK-NOT: affine.store
        // CHECK-NOT: affine.load
        // CHECK-NOT: secret.load
        // CHECK-NOT: secret.store
        // CHECK: fhe.alloc
        // CHECK: fhe.load
        // CHECK: memref.load
        // CHECK: fhe.lwemulplain
        // CHECK: fhe.store
    }
}

