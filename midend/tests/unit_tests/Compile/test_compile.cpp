
#include <iostream>
#include <string_view>
#include <string>
#include "Runtime/CompilerEngine.h"

using namespace mlir;
using namespace aegis;


constexpr std::string_view prog_content = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.mulf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";

constexpr std::string_view prog_content_2 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %1 = arith.mulf %arg0, %arg1 :  f32
        %2 = arith.mulf %1, %arg0 : f32
        %3 = arith.mulf %1, %2 : f32
        %4 = arith.mulf %2, %3 : f32
        %5 = arith.mulf %3, %4 : f32
        %6 = arith.mulf %4, %5 : f32
        %7 = arith.mulf %5, %6 : f32
        %8 = arith.mulf %6, %7 : f32
        %9 = arith.mulf %7, %8 : f32
        return %9 : f32
    }
}
)mlir";

constexpr std::string_view prog_content_3 = R"mlir(
module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "add"} {
  func.func @main_graph(%arg0: memref<3x2xf32> {onnx.name = "X1", onnx.type = "encrypted"}, 
                        %arg1: memref<3x2xf32> {onnx.name = "X2", onnx.type = "encrypted"}) 
                        -> (memref<3x2xf32> {onnx.name = "Y"}) attributes {llvm.emit_c_interface} {
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
)mlir";

constexpr std::string_view prog_content_4 = R"mlir(
module {
  func.func @add_1d_6elements(%arg0: memref<6xf32>, %arg1: memref<6xf32>) -> memref<6xf32> {
    affine.for %arg2 = 0 to 6 {
      %0 = affine.load %arg0[%arg2] : memref<6xf32>
      %1 = affine.load %arg1[%arg2] : memref<6xf32>
      %2 = arith.addf %0, %1 : f32
      affine.store %2, %arg0[%arg2] : memref<6xf32>
    }
    return %arg0 : memref<6xf32>
  }
}
)mlir";

constexpr std::string_view prog_content_5 = R"mlir(
module {
  func.func @main_graph(%arg0: memref<6xf32>, %arg1: memref<6xf32>) -> f32 {
    %cst = arith.constant 0.000000e+00 : f32
    %0 = affine.for %arg2 = 0 to 6 iter_args(%arg3 = %cst) -> (f32) {
      %1 = affine.load %arg0[%arg2] : memref<6xf32>
      %2 = affine.load %arg1[%arg2] : memref<6xf32>
      %3 = arith.addf %1, %2 : f32
      %4 = arith.addf %arg3, %3 : f32
      affine.yield %4 : f32
    }
    return %0 : f32
  }
}
)mlir";


bool compileMlir(std::string_view mlirContent) {
    auto compile_context =  CompileContext::createContext();
    CompilerEngine engine(compile_context);
    CompileOptions compileOpts = engine.getCompileOptions();
    compileOpts.target = TARGET::LOWER_MLIR;
    // compileOpts.verbose = true;
    engine.setCompileOptions(compileOpts);
    auto compile_res = engine.compile(mlirContent);
    if (!compile_res) {
        return false;
    }

    compileOpts.target = TARGET::SECRET;
    engine.setCompileOptions(compileOpts);
    compile_res = engine.compile(mlirContent);
    if (!compile_res) {
        return false;
    }

    compileOpts.target = TARGET::FHE;
    engine.setCompileOptions(compileOpts);
    compile_res = engine.compile(mlirContent);
    if (!compile_res) {
        return false;
    }

    compileOpts.target = TARGET::EMITC;
    engine.setCompileOptions(compileOpts);
    compile_res = engine.compile(mlirContent);
    if (!compile_res) {
        return false;
    }

    compileOpts.target = TARGET::CPP;
    engine.setCompileOptions(compileOpts);
    compile_res = engine.compile(mlirContent);
    if (!compile_res) {
        return false;
    }

    compileOpts.target = TARGET::LIBRARY;
    engine.setCompileOptions(compileOpts);
    compile_res = engine.compile(mlirContent);
    if (!compile_res) {
        return false;
    }

    return true;
}


bool run_case(std::string_view mlirContent) {
    if (compileMlir(mlirContent)) {
        std::cout << "Test Pass." << std::endl;
        return true;
    } else {
        std::cout << "Test failure." << std::endl;
        return false;
    }
}


int main() {
    if (run_case(prog_content)) {
        return -1;
    }

    if (run_case(prog_content_2)) {
        return -1;
    }

    if (run_case(prog_content_3)) {
        return -1;
    }

    if (run_case(prog_content_4)) {
        return -1;
    }

    if (run_case(prog_content_5)) {
        return -1;
    }

    return 0;
}