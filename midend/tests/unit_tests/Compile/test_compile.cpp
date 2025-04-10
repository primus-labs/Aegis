
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


int main() {
    
    if (compileMlir(prog_content)) {
        std::cout << "Test Pass." << std::endl;
    } else {
        std::cout << "Test failure." << std::endl;
    }

    return 0;
}