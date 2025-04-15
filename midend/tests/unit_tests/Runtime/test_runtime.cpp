#include <iostream>
#include <string_view>
#include <string>
#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHERuntime.h"
#include "Common/Value.h"
#include "Common/ProgramSpec.h"
#include "cpu/FHE/include/KeysetGenerator.h"
#include "cpu/FHE/include/Operate.h"

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
    {
        // auto compile_context =  CompileContext::createContext();
        // CompilerEngine engine(compile_context);
        // CompileOptions compileOpts = engine.getCompileOptions();
        // compileOpts.target = TARGET::LIBRARY;
        // engine.setCompileOptions(compileOpts);
        // auto compile_res = engine.compile(mlirContent);
        // if (!compile_res) {
        //     return false;
        // }

        // FHERuntime runtimeObj((*compile_res).progSpecFileName);
        // if (!runtimeObj.open((*compile_res).binFileName)) {
        //     std::cout << "Test failure." << std::endl;
        // }

        // if (!runtimeObj.resolveSymbol("main_graph")) {
        //     std::cout << "Test failure." << std::endl;
        // }

        // // Generate keygen
        // ProgramSpec &progSpecObj = ProgramSpec::getInstance();
        // if (!progSpecObj.initialize("/tmp/aegis/prog_spec.json")) {
        //      std::cout << "Test failure." << std::endl;
        //     return -1;
        // }
        // ProtoMessage<aegisprotocol::KeyInfo> keyInfos = progSpecObj.getKeyInfo();
        // aegiscpu::KeysetGenerator::generate(keyInfos);

        // // Encrypte data & Run & Decrypte data
        // auto encryptRunDecrypt = [&](std::vector<double> a, std::vector<double> b, size_t resSizes) -> std::vector<double> {
        //     std::vector<uint8_t> cipher_a = aegiscpu::encrypt({a});
        //     std::vector<uint8_t> cipher_b = aegiscpu::encrypt({b});

        //     // Run
        //     size_t dims = 0;
        //     if (resSizes > 1)
        //         dims = 1;
        //     mlir::aegis::Value value_a(Tensor<uint8_t>(cipher_a, std::vector<size_t>{dims}));
        //     mlir::aegis::Value value_b(Tensor<uint8_t>(cipher_b, std::vector<size_t>{dims}));
        //     std::vector<mlir::aegis::Value> params;
        //     params.push_back(value_a);
        //     params.push_back(value_b);
        //     auto resOrErr = runtimeObj.call(params);
        //     if (!resOrErr) {
        //         std::cout << "Test failure." << std::endl;
        //         exit(-1);
        //     }
        //     std::vector<mlir::aegis::Value> res = *resOrErr;

        //     // Decrypte data
        //     return aegiscpu::decrypt(res[0].getTensor<uint8_t>().value().values, resSizes);
        // };

        // // case 1:
        // double a = 3.0;
        // double b = 4.0;
        // std::vector<double> res = encryptRunDecrypt({a}, {b}, 1);
        // std::cout << "3.0 * 4.0 = " << res[0] << std::endl;

        // // case 2:
        // double a2 = 3.3;
        // double b2 = 4.4;
        // res = encryptRunDecrypt({a2}, {b2}, 1);
        // std::cout << "3.3 * 4.4 = " << res[0] << std::endl;

        // // case 3:
        // std::vector<double> a3{2.2, 3.3};
        // std::vector<double> b3{4.4, 5.5};
        // res = encryptRunDecrypt(a3, b3, 2);
        // std::cout << "(2.2, 3.3) * 4.4, 5.5) = " << res[0]  << "," << res[1] << std::endl;
        // std::cout << "Test Pass." << std::endl;
        // return 0;
    }
    {
        FHERuntime runtimeObj("/tmp/aegis/prog_spec.json");
        if (!runtimeObj.open("/tmp/aegis/libtest.so")) {
            std::cout << "Test failure." << std::endl;
            return -1;
        }

        if (!runtimeObj.resolveSymbol("main_graph")) {
            std::cout << "Test failure." << std::endl;
            return -1;
        }

        // Generate keygen
        ProgramSpec &progSpecObj = ProgramSpec::getInstance();
        if (!progSpecObj.initialize("/tmp/aegis/prog_spec.json")) {
             std::cout << "Test failure." << std::endl;
            return -1;
        }
        ProtoMessage<aegisprotocol::KeyInfo> keyInfos = progSpecObj.getKeyInfo();
        aegiscpu::KeysetGenerator::generate(keyInfos);

        // Encrypte data & Run & Decrypte data
        auto encryptRunDecrypt = [&](std::vector<double> a, std::vector<double> b, size_t resSizes) -> std::vector<double> {
            std::vector<uint8_t> cipher_a = aegiscpu::encrypt({a});
            std::vector<uint8_t> cipher_b = aegiscpu::encrypt({b});

            // Run
            size_t dims = 0;
            if (resSizes > 1)
                dims = 1;
            mlir::aegis::Value value_a(Tensor<uint8_t>(cipher_a, std::vector<size_t>{dims}));
            mlir::aegis::Value value_b(Tensor<uint8_t>(cipher_b, std::vector<size_t>{dims}));
            std::vector<mlir::aegis::Value> params;
            params.push_back(value_a);
            params.push_back(value_b);
            auto resOrErr = runtimeObj.call(params);
            if (!resOrErr) {
                std::cout << "Test failure." << std::endl;
                exit(-1);
            }
            std::vector<mlir::aegis::Value> res = *resOrErr;

            // Decrypte data
            return aegiscpu::decrypt(res[0].getTensor<uint8_t>().value().values, resSizes);
        };

        // case 1:
        double a = 3.0;
        double b = 4.0;
        std::vector<double> res = encryptRunDecrypt({a}, {b}, 1);
        std::cout << "3.0 * 4.0 = " << res[0] << std::endl;

        // case 2:
        double a2 = 3.3;
        double b2 = 4.4;
        res = encryptRunDecrypt({a2}, {b2}, 1);
        std::cout << "3.3 * 4.4 = " << res[0] << std::endl;

        // case 3:
        std::vector<double> a3{2.2, 3.3};
        std::vector<double> b3{4.4, 5.5};
        res = encryptRunDecrypt(a3, b3, 2);
        std::cout << "(2.2, 3.3) * 4.4, 5.5) = " << res[0]  << "," << res[1] << std::endl;

        std::cout << "Test Pass." << std::endl;
        return 0;
    }
}