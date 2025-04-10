
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
    auto compile_res = engine.compile(mlirContent, TARGET::LOWER_MLIR);
    if (!compile_res) {
        return false;
    }

    compile_res = engine.compile(mlirContent, TARGET::SECRET);
    if (!compile_res) {
        return false;
    }

    compile_res = engine.compile(mlirContent, TARGET::FHE);
    if (!compile_res) {
        return false;
    }

    compile_res = engine.compile(mlirContent, TARGET::EMITC);
    if (!compile_res) {
        return false;
    }

    compile_res = engine.compile(mlirContent, TARGET::CPP);
    if (!compile_res) {
        return false;
    }

    compile_res = engine.compile(mlirContent, TARGET::LIBRARY);
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