#include <iostream>
#include <string_view>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHERuntime.h"
#include "Common/Value.h"
#include "Common/ProgramSpec.h"
#include "cpu/FHE/include/KeysetGenerator.h"
#include "cpu/FHE/include/Operate.h"

using namespace mlir;
using namespace aegis;


template<typename T>
bool approximatelyEqual(T a, T b, T absEpsilon = std::numeric_limits<T>::epsilon(), 
                                  T relEpsilon = std::numeric_limits<T>::epsilon()) {
    static_assert(std::is_floating_point<T>::value, "approximatelyEqual requires floating point types");
    if (std::isnan(a) || std::isnan(b)) {
        return false;
    }
    
    if (std::isinf(a) || std::isinf(b)) {
        return a == b;
    }
    
    if (std::signbit(a) != std::signbit(b)) {
        return std::abs(a) <= absEpsilon && std::abs(b) <= absEpsilon;
    }
    
    T diff = std::abs(a - b);
    if (diff <= absEpsilon) {
        return true;
    }
    
    T maxVal = std::max(std::abs(a), std::abs(b));
    return diff <= relEpsilon * maxVal;
}


std::vector<double> encryptRunDecrypt(std::shared_ptr<FHERuntime> runtime, std::vector<double> a, size_t resSizes) {
    assert(runtime);
    //Encrypt
    std::vector<uint8_t> cipher_a = aegiscpu::encrypt(a);

    // Run
    size_t dims = 0;
    if (resSizes > 1)
        dims = 1;
    mlir::aegis::Value value_a(Tensor<uint8_t>(cipher_a, std::vector<size_t>{dims}));
    std::vector<mlir::aegis::Value> params;
    params.push_back(value_a);
    auto resOrErr = runtime->call(params);
    if (!resOrErr) {
        std::cout << "call FHERuntime::call fail" << std::endl;
        exit(-1);
    }
    std::vector<mlir::aegis::Value> res = *resOrErr;

    // Decrypte data
    return aegiscpu::decrypt(res[0].getTensor<uint8_t>().value().values, resSizes);
}

std::vector<double> encryptRunDecrypt_2(std::shared_ptr<FHERuntime> runtime, std::vector<double> a, std::vector<double> b, size_t resSizes) {
    assert(runtime);
    //Encrypt
    std::vector<uint8_t> cipher_a = aegiscpu::encrypt(a);
    std::vector<uint8_t> cipher_b = aegiscpu::encrypt(b);

    // Run
    size_t dims = 0;
    if (resSizes > 1)
        dims = 1;
    mlir::aegis::Value value_a(Tensor<uint8_t>(cipher_a, std::vector<size_t>{dims}));
    mlir::aegis::Value value_b(Tensor<uint8_t>(cipher_b, std::vector<size_t>{dims}));
    std::vector<mlir::aegis::Value> params;
    params.push_back(value_a);
    params.push_back(value_b);
    auto resOrErr = runtime->call(params);
    if (!resOrErr) {
        std::cout << "call FHERuntime::call fail" << std::endl;
        exit(-1);
    }
    std::vector<mlir::aegis::Value> res = *resOrErr;

    // Decrypte data
    return aegiscpu::decrypt(res[0].getTensor<uint8_t>().value().values, resSizes);
}

std::shared_ptr<FHERuntime> CompileAndOpenSymbol(const std::string_view & mlirStr, const std::string &funcName) {
    auto compile_context =  CompileContext::createContext();
    CompilerEngine engine(compile_context);
    CompileOptions compileOpts = engine.getCompileOptions();
    compileOpts.target = TARGET::LIBRARY;
    engine.setCompileOptions(compileOpts);
    auto compile_res = engine.compile(mlirStr);
    if (!compile_res) {
        std::cout << "compile fail" << std::endl;
        return nullptr;
    }

    std::shared_ptr<FHERuntime> pRuntime = std::make_shared<FHERuntime>((*compile_res).progSpecFileName);
    if (!pRuntime->open((*compile_res).binFileName)) {
        std::cout << "call open fail" << std::endl;
        return nullptr;
    }

    if (!pRuntime->resolveSymbol(funcName)) {
        std::cout << "call resolveSymbol fail" << std::endl;
        return nullptr;
    }

    // Generate keygen
    ProgramSpec &progSpecObj = ProgramSpec::getInstance();
    if (!progSpecObj.initialize("/tmp/aegis/prog_spec.json")) {
         std::cout << "call ProgramSpec::initialize fail" << std::endl;
        return nullptr;
    }
    ProtoMessage<aegisprotocol::KeyInfo> keyInfos = progSpecObj.getKeyInfo();
    aegiscpu::KeysetGenerator::generate(keyInfos);

    return pRuntime;
}

bool mlirUnitTest(const std::string_view & mlirStr, const std::vector<double> &input, const std::vector<double> &expect_output) {
    auto runtime = CompileAndOpenSymbol(mlirStr, "main_graph");
    if (runtime == nullptr) {
        return false;
    }

    std::vector<double> output = encryptRunDecrypt(runtime, input, expect_output.size());
    for (size_t i = 0; i < output.size(); i++) {
        if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
            return false;
        }
    }

    return true;
}

bool mlirUnitTest_2(const std::string_view & mlirStr, const std::vector<double> &input_a, const std::vector<double> &input_b, 
                    const std::vector<double> &expect_output) {
    auto runtime = CompileAndOpenSymbol(mlirStr, "main_graph");
    if (runtime == nullptr) {
        return false;
    }

    std::vector<double> output = encryptRunDecrypt_2(runtime, input_a, input_b, expect_output.size());
    for (auto i = 0; i < output.size(); i++) {
        if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
            return false;
        }
    }

    return true;
}

/**********************************
***********    case 1    **********
***********************************/
constexpr std::string_view mlirCase1 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.mulf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";

bool case_1() {
    double a = 3.0;
    double b = 4.0;
    std::vector<double> expect_output(1, a*b);
    if (!mlirUnitTest_2(mlirCase1, {a}, {b}, expect_output)) {
        return false;
    }

    double a2 = 3.3;
    double b2 = 4.4;
    std::vector<double> expect_output2(1, a2*b2);
    if (!mlirUnitTest_2(mlirCase1, {a2}, {b2}, expect_output2)) {
        return false;
    }

    std::vector<double> a3{2.2, 3.3};
    std::vector<double> b3{4.4, 5.5};
    std::vector<double> expect_output3;
    expect_output3.push_back(a3[0]*b3[0]);
    expect_output3.push_back(a3[1]*b3[1]);
    if (!mlirUnitTest_2(mlirCase1, {a3}, {b3}, expect_output3)) {
        return false;
    }

    return true;
}


/**********************************
***********    case 2    **********
***********************************/
constexpr std::string_view mlirCase2 = R"mlir(
module  {
  func.func  @MVP(%m: memref<16xf64> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                  %v: memref<4xf64>  {onnx.name = "input_y", onnx.type = "encrypted"}
                  ) -> memref<4xf64> {
    %c0 = arith.constant 0 : index
    %c4 = arith.constant 4 : index
    %c0_sf64 = arith.constant 0.000000e+00 : f64
    // for each row in matrix
    %0 = affine.for %i = 0 to 2 iter_args(%r = %v) -> (memref<4xf64>) {
      // iterate over the vector
      %1 = affine.for %j = 0 to 2 iter_args(%sum = %c0_sf64) -> (f64) {
       // compute ij = i*4 + j
       %2 = arith.muli %i, %c4 : index
       %ij = arith.addi %2, %j : index
        %mij = memref.load %m[%ij] : memref<16xf64>
        %vj = memref.load %v[%j] : memref<4xf64>
        %p = arith.mulf %mij, %vj  : f64
        %s = arith.addf %sum, %p : f64
        affine.yield %s : f64
      }

      memref.store %1, %r[%i] : memref<4xf64>
      affine.yield %r : memref<4xf64>
    }
    return %0: memref<4xf64>
  }
}
)mlir";

bool case_2() {
    std::vector<double> m = {1.0, 2.0, 0.0, 0.0, 3.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> v = {2.0, 2.0, 5.0, 6.0};
    std::vector<double> expect_output = {6.0, 14.0, 5.0, 6.0};
    if (!mlirUnitTest_2(mlirCase2, m, v, expect_output)) {
        return false;
    }

    std::vector<double> m2 = {2.0, 3.0, 0.0, 0.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> v2 = {1.0, 0.0, 100.0, 200.0};
    std::vector<double> expect_output2 = {2.0, 5.0, 100.0, 200.0};
    if (!mlirUnitTest_2(mlirCase2, m2, v2, expect_output2)) {
        return false;
    }

    std::vector<double> m3 = {2.0, 3.0, 1.0, 1.0, 4.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> v3 = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> expect_output3 = {8.0, 14.0, 3.0, 4.0};
    if (!mlirUnitTest_2(mlirCase2, m3, v3, expect_output3)) {
        return false;
    }

    return true;
}


//----------------------------------------------------------------
int main() {
    {
        if (!case_1()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_2()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        std::cout << "Test pass" << std::endl;
        return 0;
    }

    {
        std::shared_ptr<FHERuntime> pRuntime = std::make_shared<FHERuntime>("/tmp/aegis/prog_spec.json");
        if (!pRuntime->open("/tmp/aegis/libtest.so")) {
            std::cout << "Test failure." << std::endl;
            return -1;
        }

        if (!pRuntime->resolveSymbol("main_graph")) {
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

        // case 1:
        {
            // scene 1:
            double a = 3.0;
            double b = 4.0;
            std::vector<double> expect_output(1, a*b);
            std::vector<double> output = encryptRunDecrypt_2(pRuntime, {a}, {b}, expect_output.size());
            for (auto i = 0; i < output.size(); i++) {
                if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }

            // scene 2:
            double a2 = 3.3;
            double b2 = 4.4;
            std::vector<double> expect_output2(1, a2*b2);
            std::vector<double> output2 = encryptRunDecrypt_2(pRuntime, {a2}, {b2}, expect_output2.size());
            for (auto i = 0; i < output2.size(); i++) {
                if (!approximatelyEqual(output2[i], expect_output2[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }

            // scene 3:
            std::vector<double> a3{2.2, 3.3};
            std::vector<double> b3{4.4, 5.5};
            std::vector<double> expect_output3 = {2.2*4.4, 3.3*5.5};
            std::vector<double> output3 = encryptRunDecrypt_2(pRuntime, a3, b3, expect_output3.size());
            for (auto i = 0; i < output3.size(); i++) {
                if (!approximatelyEqual(output3[i], expect_output3[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }
        }

        // case 2:
        {
            // scene 1:
            std::vector<double> m = {1.0, 2.0, 0.0, 0.0, 3.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> v = {2.0, 2.0, 5.0, 6.0};
            std::vector<double> expect_output = {6.0, 14.0, 5.0, 6.0};
            std::vector<double> output = encryptRunDecrypt_2(pRuntime, m, v, expect_output.size());
            for (auto i = 0; i < output.size(); i++) {
                if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }

            // scene 2:
            std::vector<double> m2 = {2.0, 3.0, 0.0, 0.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> v2 = {1.0, 0.0, 100.0, 200.0};
            std::vector<double> expect_output2 = {2.0, 5.0, 100.0, 200.0};
            std::vector<double> output2 = encryptRunDecrypt_2(pRuntime, m2, v2, expect_output2.size());
            for (auto i = 0; i < output2.size(); i++) {
                if (!approximatelyEqual(output2[i], expect_output2[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }

            // scene 3:
            std::vector<double> m3 = {2.0, 3.0, 1.0, 1.0, 4.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> v3 = {1.0, 2.0, 3.0, 4.0};
            std::vector<double> expect_output3 = {8.0, 14.0, 3.0, 4.0};
            std::vector<double> output3 = encryptRunDecrypt_2(pRuntime, m3, v3, expect_output3.size());
            for (auto i = 0; i < output3.size(); i++) {
                if (!approximatelyEqual(output3[i], expect_output3[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }
        }

        std::cout << "Test pass." << std::endl;
        return 0;
    }
}
