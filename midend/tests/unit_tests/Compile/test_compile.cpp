
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

    return 0;
}