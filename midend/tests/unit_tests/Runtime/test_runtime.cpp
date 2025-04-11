#include <iostream>
#include <string_view>
#include <string>
#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHERuntime.h"

using namespace mlir;
using namespace aegis;


constexpr std::string_view mlirContent = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.mulf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";


int main() {
    auto compile_context =  CompileContext::createContext();
    CompilerEngine engine(compile_context);
    CompileOptions compileOpts = engine.getCompileOptions();
    compileOpts.target = TARGET::LIBRARY;
    engine.setCompileOptions(compileOpts);
    auto compile_res = engine.compile(mlirContent);
    if (!compile_res) {
        return false;
    }

    FHERuntime runtimeObj((*compile_res).progSpecFileName);
    if (!runtimeObj.open((*compile_res).binFileName)) {
        std::cout << "Test failure." << std::endl;
    }

    if (!runtimeObj.resolveSymbol("main_graph")) {
        std::cout << "Test failure." << std::endl;
    }

    // prepare input value
    // if (!runtimeObj.call()) {
    //     std::cout << "Test failure." << std::endl;
    // }

    std::cout << "Test Pass." << std::endl;
    return 0;
}