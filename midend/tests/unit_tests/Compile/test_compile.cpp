
#include <iostream>
#include <string_view>
#include <string>
#include "Runtime/CompilerEngine.h"


constexpr std::string_view prog_content = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.mulf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";


using namespace mlir;
using namespace aegis;

int main() {
    auto compile_context =  CompileContext::createContext();
    CompilerEngine engine(compile_context);
    auto compile_res = engine.compile(prog_content, TARGET::LOWER_MLIR);
    if (!compile_res) {
        std::cout << "Test failure." << std::endl;
        return -1;
    }

    compile_res = engine.compile(prog_content, TARGET::SECRET);
    if (!compile_res) {
        std::cout << "Test failure." << std::endl;
        return -1;
    }

    compile_res = engine.compile(prog_content, TARGET::FHE);
    if (!compile_res) {
        std::cout << "Test failure." << std::endl;
        return -1;
    }

    compile_res = engine.compile(prog_content, TARGET::EMITC);
    if (!compile_res) {
        std::cout << "Test failure." << std::endl;
        return -1;
    }

    compile_res = engine.compile(prog_content, TARGET::CPP);
    if (!compile_res) {
        std::cout << "Test failure." << std::endl;
        return -1;
    }

    compile_res = engine.compile(prog_content, TARGET::LIBRARY);
    if (!compile_res) {
        std::cout << "Test failure." << std::endl;
        return -1;
    }

    std::cout << "Test Pass." << std::endl;
    return 0;
}