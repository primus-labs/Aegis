#include <iostream>
#include <string_view>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include "Runtime/CompilerEngine.h"
#include "Runtime/FHE/FHERuntime.h"
#include "Runtime/FHE/FHEDataProcessor.h"
#include "Common/Value.h"
#include "Common/ProgramSpec.h"
#include "cpu/FHE/include/KeysetGenerator.h"
#include "cpu/FHE/include/Operate.h"
#include "cpu/FHE/include/CryptoContextMgr.h"
#include "cpu/FHE/include/FheKeyset.h"

#include "openfhe.h"
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"
using namespace lbcrypto;

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
    FHEDataProcessor fheDataProcessor;
    std::vector<mlir::aegis::Value> inputs;
    auto inputA = fheDataProcessor.privateInput(a);
    inputs.emplace_back(inputA);

    // Run
    auto resOrErr = runtime->call(inputs);
    if (!resOrErr) {
        std::cout << "call FHERuntime::call fail" << std::endl;
        exit(-1);
    }

    // Process Output
    std::vector<double> res_db;
    std::vector<mlir::aegis::Value> res = *resOrErr;
    auto outputs = fheDataProcessor.processOutput(res);
    for (auto i = 0; i < outputs.size(); i++) {
        auto theOutput = outputs[i];
        auto item_res = theOutput.getTensor<double>().value().values;
        res_db.insert(res_db.end(), item_res.begin(), item_res.end());
    }
    
    return res_db;
}

std::vector<double> encryptRunDecrypt_2(std::shared_ptr<FHERuntime> runtime, 
                                           std::vector<double> a, bool a_is_cipher,
                                           std::vector<double> b, bool b_is_cipher) {
    assert(runtime);

    //Encrypt
    std::vector<mlir::aegis::Value> inputs;
    FHEDataProcessor fheDataProcessor;
    if (a_is_cipher) {
        auto inputA= fheDataProcessor.privateInput(a);
        inputs.emplace_back(inputA);
    } else {
        auto inputA = fheDataProcessor.publicInput(a);
        inputs.emplace_back(inputA);
    }

    if (b_is_cipher) {
        auto inputB= fheDataProcessor.privateInput(b);
        inputs.emplace_back(inputB);
    } else {
        auto inputB = fheDataProcessor.publicInput(b);
        inputs.emplace_back(inputB);
    }

    // Run
    auto resOrErr = runtime->call(inputs);
    if (!resOrErr) {
        std::cout << "call FHERuntime::call fail," << llvm::toString(resOrErr.takeError()) << std::endl;
        exit(-1);
    }

    // Process Output
    std::vector<double> res_db;
    std::vector<mlir::aegis::Value> res = *resOrErr;
    auto outputs = fheDataProcessor.processOutput(res);
    for (auto i = 0; i < outputs.size(); i++) {
        auto theOutput = outputs[i];
        auto item_res = theOutput.getTensor<double>().value().values;
        res_db.insert(res_db.end(), item_res.begin(), item_res.end());
    }

    return res_db;
}

std::vector<double> encryptRunDecrypt_2_Ex(std::shared_ptr<FHERuntime> runtime, 
                                           std::vector<std::vector<double>> a, bool a_is_cipher,
                                           std::vector<std::vector<double>> b, bool b_is_cipher) {
    assert(runtime);

    //Encrypt
    std::vector<mlir::aegis::Value> inputs;
    FHEDataProcessor fheDataProcessor;
    if (a_is_cipher) {
        auto inputA = fheDataProcessor.privateInput(a);
        inputs.insert(inputs.end(), inputA.begin(), inputA.end());
    } else {
        auto inputA = fheDataProcessor.publicInput(a);
        inputs.insert(inputs.end(), inputA.begin(), inputA.end());
    }

    if (b_is_cipher) {
        auto inputB = fheDataProcessor.privateInput(b);
        inputs.insert(inputs.end(), inputB.begin(), inputB.end());
    } else {
        auto inputB = fheDataProcessor.publicInput(b);
        inputs.insert(inputs.end(), inputB.begin(), inputB.end());
    }

    // Run
    auto resOrErr = runtime->call(inputs);
    if (!resOrErr) {
        std::cout << "call FHERuntime::call fail," << llvm::toString(resOrErr.takeError()) << std::endl;
        exit(-1);
    }

    // Process Output
    std::vector<double> res_db;
    std::vector<mlir::aegis::Value> res = *resOrErr;
    auto outputs = fheDataProcessor.processOutput(res);
    for (auto i = 0; i < outputs.size(); i++) {
        auto theOutput = outputs[i];
        auto item_res = theOutput.getTensor<double>().value().values;
        res_db.insert(res_db.end(), item_res.begin(), item_res.end());
    }

    return res_db;
}

std::shared_ptr<FHERuntime> CompileAndOpenSymbol(const std::string_view & mlirStr, const std::string &funcName) {
    auto compile_context =  CompileContext::createContext();
    CompilerEngine engine(compile_context);
    CompileOptions compileOpts = engine.getCompileOptions();
    compileOpts.target = TARGET::LIBRARY;
    engine.setCompileOptions(compileOpts);
    auto compile_res = engine.compile(mlirStr);
    if (!compile_res) {
        std::cout << "compile fail:" << llvm::toString(compile_res.takeError()) << std::endl;
        return nullptr;
    }

    std::shared_ptr<FHERuntime> pRuntime = std::make_shared<FHERuntime>("/tmp/aegis/prog_spec.json");
    if (!pRuntime->open((*compile_res).outputDirPath + (*compile_res).binFileName)) {
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
    aegiscpu::openfhe::KeysetGenerator::generate(keyInfos);

    // Serial various keys
    auto cryptoCtx = aegiscpu::openfhe::CryptoContextMgr::getInstance().getCryptoContext();
    const std::string ccFileName= "/tmp/aegis/cryptocontext.txt";
    if (!Serial::SerializeToFile(ccFileName, cryptoCtx, SerType::BINARY)) {
        std::cerr << "Error writing serialization of the crypto context to cryptocontext.txt" << std::endl;
        return nullptr;
    }

    const std::string pubKeyFileName = "/tmp/aegis/pubkey.txt";
    std::shared_ptr<aegiscpu::openfhe::FHEPublicKey> aegisPubKey = aegiscpu::openfhe::FheKeyset::getInstance().getPubKey();
    PublicKey<DCRTPoly> pubKey = aegisPubKey->getKey();
    if (!Serial::SerializeToFile(pubKeyFileName, pubKey, SerType::BINARY)) {
        std::cerr << "Exception writing public key to " << pubKeyFileName << std::endl;
        return nullptr;
    }

    const std::string mulKeyFileName = "/tmp/aegis/mulkey.txt";
    std::ofstream multKeyFile(mulKeyFileName, std::ios::out | std::ios::binary);
    if (multKeyFile.is_open()) {
        if (!cryptoCtx->SerializeEvalMultKey(multKeyFile, SerType::BINARY)) {
            std::cerr << "Error writing eval mult keys" << std::endl;
            return nullptr;
        }
        multKeyFile.close();
    }
    else {
        std::cerr << "Error serializing EvalMult keys" << std::endl;
        return nullptr;
    }

    std::string rotKeyFileName = "/tmp/aegis/rotkey.txt";
    std::vector<int> galoisIdx;
    for (auto ind : keyInfos.asReader().getGaloisIndices()) {
        galoisIdx.push_back(ind);
    }
    if (galoisIdx.size() > 0) {
        std::ofstream rotationKeyFile(rotKeyFileName, std::ios::out | std::ios::binary);
        if (rotationKeyFile.is_open()) {
            if (!cryptoCtx->SerializeEvalAutomorphismKey(rotationKeyFile, SerType::BINARY)) {
                std::cerr << "Error writing rotation keys" << std::endl;
                return nullptr;
            }
            rotationKeyFile.close();
        }
        else {
            std::cerr << "Error serializing Rotation keys" << std::endl;
            return nullptr;
        }
    } else {
        rotKeyFileName = ""; //mean not has galois key
    }

    // call loadCryptoResources func
    if (!pRuntime->loadCryptoResources(ccFileName, pubKeyFileName, mulKeyFileName, rotKeyFileName)) {
        std::cout << "Test failure." << std::endl;
        return nullptr;
    }

    return pRuntime;
}

bool mlirUnitTest(const std::string_view & mlirStr, const std::vector<double> &input, const std::vector<double> &expect_output,
                  const std::string &funcName = "main_graph") {
    auto runtime = CompileAndOpenSymbol(mlirStr, funcName);
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

bool mlirUnitTest_2(const std::string_view & mlirStr, const std::vector<double> &input_a, bool a_is_cipher,
                    const std::vector<double> &input_b, bool b_is_cipher,
                    const std::vector<double> &expect_output, const std::string &funcName = "main_graph") {
    auto runtime = CompileAndOpenSymbol(mlirStr, funcName);
    if (runtime == nullptr) {
        return false;
    }

    std::vector<double> output = encryptRunDecrypt_2(runtime, input_a, a_is_cipher,
                                                        input_b, b_is_cipher);
    for (auto i = 0; i < output.size(); i++) {
        if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
            return false;
        }
    }

    return true;
}

bool mlirUnitTest_2_Ex(const std::string_view & mlirStr, 
                       const std::vector<std::vector<double>> &input_a, bool a_is_cipher,
                       const std::vector<std::vector<double>> &input_b, bool b_is_cipher,
                       const std::vector<double> &expect_output, const std::string &funcName = "main_graph") {
    auto runtime = CompileAndOpenSymbol(mlirStr, funcName);
    if (runtime == nullptr) {
        return false;
    }

    std::vector<double> output = encryptRunDecrypt_2_Ex(runtime, input_a, a_is_cipher,
                                                        input_b, b_is_cipher);
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

constexpr std::string_view mlirCase1_1 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %5 = arith.mulf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";

bool case_1() {
    double a = 3.0;
    double b = 4.0;
    std::vector<double> expect_output(1, a*b);
    // if (!mlirUnitTest_2(mlirCase1, {a}, true, {b}, true, expect_output)) {
    //     return false;
    // }
    if (!mlirUnitTest_2(mlirCase1_1, {a}, true, {b}, false, expect_output)) {
        return false;
    }

    double a2 = 3.3;
    double b2 = 4.4;
    std::vector<double> expect_output2(1, a2*b2);
    if (!mlirUnitTest_2(mlirCase1, {a2}, true, {b2}, true, expect_output2)) {
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
    std::vector<double> v = {2.0, 2.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> expect_output = {6.0, 26.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};      
    if (!mlirUnitTest_2(mlirCase2, m, true, v, true, expect_output)) {
        return false;
    }

    std::vector<double> m2 = {2.0, 3.0, 0.0, 0.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> v2 = {1.0, 0.0, 100.0, 200.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> expect_output2 = {2.0, 10.0, 100.0, 200.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};      
    if (!mlirUnitTest_2(mlirCase2, m2, true, v2, true, expect_output2)) {
        return false;
    }

    std::vector<double> m3 = {2.0, 3.0, 1.0, 1.0, 4.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> v3 = {1.0, 2.0, 3.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::vector<double> expect_output3 = {8.0, 42.0, 3.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};     
    if (!mlirUnitTest_2(mlirCase2, m3, true, v3, true, expect_output3)) {
        return false;
    }

    return true;
}


/**********************************
***********    case 3    **********
***********************************/
constexpr std::string_view mlirCase3 = R"mlir(
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

bool case_3() {
    std::vector<double> a1 = {1.0};
    std::vector<double> b1 = {2.0};
    std::vector<double> expect_output = {17179869184.0};      
    if (!mlirUnitTest_2(mlirCase3, a1, true, b1, true, expect_output)) {
        return false;
    }

    std::vector<double> a2 = {1.0, 1.0};
    std::vector<double> b2 = {1.0, 2.0};
    std::vector<double> expect_output2 = {1.0, 17179869184.0};      
    if (!mlirUnitTest_2(mlirCase3, a2, true, b2, true, expect_output2)) {
        return false;
    }

    return true;
}


/**********************************
***********    case 4    **********
***********************************/
constexpr std::string_view mlirCase4 = R"mlir(
module {
  func.func @main_graph(%arg0: memref<1xf32> , %arg1: memref<1xf32> ) -> (memref<1xf32> ) {
    %c0 = arith.constant 0 : index
    %alloc = memref.alloc() {alignment = 16 : i64} : memref<1xf32>
    %0 = affine.load %arg0[%c0] : memref<1xf32>
    %1 = affine.load %arg1[%c0] : memref<1xf32>
    %2 = arith.addf %0, %1 : f32
    affine.store %2, %alloc[%c0] : memref<1xf32>
    return %alloc : memref<1xf32>
  }
}
)mlir";


bool case_4() {
    std::vector<double> a1 = {10.0};
    std::vector<double> b1 = {14.0};
    std::vector<double> expect_output = {24.0};      
    if (!mlirUnitTest_2(mlirCase4, a1, true, b1, true, expect_output)) {
        return false;
    }

    return true;
}


/**********************************
***********    case 5    **********
***********************************/
constexpr std::string_view mlirCase5 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %cond = arith.cmpf "olt", %arg0, %arg1 : f32
        %result = arith.select %cond, %arg0, %arg1 : f32
        return %result : f32
    }
}
)mlir";

constexpr std::string_view mlirCase5_2 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %cond = arith.cmpf "ogt", %arg0, %arg1 : f32
        %result = arith.select %cond, %arg0, %arg1 : f32
        return %result : f32
    }
}
)mlir";

constexpr std::string_view mlirCase5_3 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %cond = arith.cmpf "oeq", %arg0, %arg1 : f32
        %result = arith.select %cond, %arg0, %arg1 : f32
        return %result : f32
    }
}
)mlir";

bool case_5() {
    std::vector<double> a1 = {-2.0};
    std::vector<double> b1 = {-4.0};
    std::vector<double> expect_output = {-4.0};      
    if (!mlirUnitTest_2(mlirCase5, a1, true, b1, true, expect_output)) {
        return false;
    }

    std::vector<double> a2 = {2.0};
    std::vector<double> b2 = {4.0};
    std::vector<double> expect_output2 = {2.0};      
    if (!mlirUnitTest_2(mlirCase5, a2, true, b2, true, expect_output2)) {
        return false;
    }

    return true;
}

bool case_5_2() {
    std::vector<double> a1 = {-2.0};
    std::vector<double> b1 = {-4.0};
    std::vector<double> expect_output = {-2.0};      
    if (!mlirUnitTest_2(mlirCase5_2, a1, true, b1, true, expect_output)) {
        return false;
    }

    std::vector<double> a2 = {2.0};
    std::vector<double> b2 = {4.0};
    std::vector<double> expect_output2 = {4.0};      
    if (!mlirUnitTest_2(mlirCase5_2, a2, true, b2, true, expect_output2)) {
        return false;
    }

    return true;
}

bool case_5_3() {
    std::vector<double> a1 = {5.0};
    std::vector<double> b1 = {-5.0};
    std::vector<double> expect_output = {-5.0};      
    if (!mlirUnitTest_2(mlirCase5_3, a1, true, b1, true, expect_output)) {
        return false;
    }

    std::vector<double> a2 = {4.0};
    std::vector<double> b2 = {4.0};
    std::vector<double> expect_output2 = {4.0};      
    if (!mlirUnitTest_2(mlirCase5_3, a2, true, b2, true, expect_output2)) {
        return false;
    }

    return true;
}


/**********************************
***********    case 6    **********
***********************************/
constexpr std::string_view mlirCase6 = R"mlir(
module {
  func.func @main_graph(%arg0: memref<6xf32>, %arg1: memref<6xf32>) -> memref<6xf32> {
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

constexpr std::string_view mlirCase6_2 = R"mlir(
module {
  func.func @main_graph(%arg0: memref<6xf32> {onnx.name = "input_x", onnx.type = "encrypted"}, 
                        %arg1: memref<6xf32> {onnx.name = "input_y", onnx.type = "clear"}) -> memref<6xf32> {
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

bool case_6() {
    std::vector<double> a1 = {1, 3, 5, 7, 9, 11};
    std::vector<double> b1 = {2, 4, 6, 8, 10, 12};
    std::vector<double> expect_output = {3, 7, 11, 15, 19, 23};      
    if (!mlirUnitTest_2(mlirCase6, a1, true, b1, true, expect_output)) {
        return false;
    }

    return true;
}

bool case_6_2() {
    std::vector<double> a1 = {1, 3, 5, 7, 9, 11};
    std::vector<double> b1 = {2, 4, 6, 8, 10, 12};
    std::vector<double> expect_output = {3, 7, 11, 15, 19, 23};      
    if (!mlirUnitTest_2(mlirCase6_2, a1, true, b1, false, expect_output)) {
        return false;
    }

    return true;
}


/**********************************
***********    case 7    **********
***********************************/
constexpr std::string_view mlirCase7 = R"mlir(
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

bool case_7() {
    std::vector<double> a1 = {1, 3, 5, 7, 9, 11};
    std::vector<double> b1 = {2, 4, 6, 8, 10, 12};
    std::vector<double> expect_output = {78};      
    if (!mlirUnitTest_2(mlirCase7, a1, true, b1, true, expect_output)) {
        return false;
    }

    return true;
}


/**********************************
***********    case 8    **********
***********************************/
constexpr std::string_view mlirCase8 = R"mlir(
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

constexpr std::string_view mlirCase8_2 = R"mlir(
module attributes {llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128", llvm.target_triple = "x86_64-unknown-linux-gnu", "onnx-mlir.symbol-postfix" = "add"} {
  func.func @main_graph(%arg0: memref<3x2xf32> {onnx.name = "X1", onnx.type = "encrypted"}, 
                        %arg1: memref<3x2xf32> {onnx.name = "X2", onnx.type = "clear"}) 
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

bool case_8() {
    std::vector<std::vector<double>> a1 = {{1, 3}, {5, 7}, {9,  11}};
    std::vector<std::vector<double>> b1 = {{2, 4}, {6, 8}, {10, 12}};
    std::vector<double> expect_output = {3, 7, 11, 15, 19, 23};      
    if (!mlirUnitTest_2_Ex(mlirCase8, a1, true, b1, true, expect_output)) {
        return false;
    }

    return true;
}

bool case_8_2() {
    std::vector<std::vector<double>> a1 = {{1, 3}, {5, 7}, {9,  11}};
    std::vector<std::vector<double>> b1 = {{2, 4}, {6, 8}, {10, 12}};
    std::vector<double> expect_output = {3, 7, 11, 15, 19, 23};      
    if (!mlirUnitTest_2_Ex(mlirCase8_2, a1, true, b1, false, expect_output)) {
        return false;
    }

    return true;    
}

/**********************************
***********    case 9    **********
***********************************/
constexpr std::string_view mlirCase9 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";

constexpr std::string_view mlirCase9_2 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "encrypted"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "clear"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";

constexpr std::string_view mlirCase9_3 = R"mlir(
module {
    func.func @main_graph(%arg0: f32 {onnx.name = "input_x", onnx.type = "clear"}, 
                          %arg1: f32 {onnx.name = "input_y", onnx.type = "encrypted"}) -> f32 {
        %5 = arith.divf %arg0, %arg1 :  f32
        return %5 : f32
    }
}
)mlir";

bool case_9() {
    std::vector<double> a1 = {1.1};
    std::vector<double> b1 = {3.1};
    std::vector<double> expect_output = {0.3548};      
    if (!mlirUnitTest_2(mlirCase9, a1, true, b1, true, expect_output)) {
        return false;
    }

    if (!mlirUnitTest_2(mlirCase9_2, a1, true, b1, false, expect_output)) {
        return false;
    }

    if (!mlirUnitTest_2(mlirCase9_3, a1, false, b1, true, expect_output)) {
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

        if (!case_3()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_4()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_5()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_5_2()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_5_3()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_6()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_6_2()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_7()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_8()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_8_2()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        if (!case_9()) {
            std::cout << "Test fail" << std::endl;
            return -1;
        }

        std::cout << "Test pass" << std::endl;
        return 0;
    }

    /*
    {
        std::string funcName = "main_graph";
        std::shared_ptr<FHERuntime> pRuntime = std::make_shared<FHERuntime>("/tmp/aegis/prog_spec.json");
        if (!pRuntime->open("/tmp/aegis/libtest.so")) {
            std::cout << "Test failure." << std::endl;
            return -1;
        }

        if (!pRuntime->resolveSymbol(funcName)) {
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
        aegiscpu::openfhe::KeysetGenerator::generate(keyInfos);

        // Serial various keys
        auto cryptoCtx = aegiscpu::openfhe::CryptoContextMgr::getInstance().getCryptoContext();
        const std::string ccFileName= "/tmp/aegis/cryptocontext.txt";
        if (!Serial::SerializeToFile(ccFileName, cryptoCtx, SerType::BINARY)) {
            std::cerr << "Error writing serialization of the crypto context to cryptocontext.txt" << std::endl;
            return -1;
        }

        const std::string pubKeyFileName = "/tmp/aegis/pubkey.txt";
        std::shared_ptr<aegiscpu::openfhe::FHEPublicKey> aegisPubKey = aegiscpu::openfhe::FheKeyset::getInstance().getPubKey();
        PublicKey<DCRTPoly> pubKey = aegisPubKey->getKey();
        if (!Serial::SerializeToFile(pubKeyFileName, pubKey, SerType::BINARY)) {
            std::cerr << "Exception writing public key to " << pubKeyFileName << std::endl;
            return -1;
        }

        const std::string mulKeyFileName = "/tmp/aegis/mulkey.txt";
        std::ofstream multKeyFile(mulKeyFileName, std::ios::out | std::ios::binary);
        if (multKeyFile.is_open()) {
            if (!cryptoCtx->SerializeEvalMultKey(multKeyFile, SerType::BINARY)) {
                std::cerr << "Error writing eval mult keys" << std::endl;
                return -1;
            }
            multKeyFile.close();
        }
        else {
            std::cerr << "Error serializing EvalMult keys" << std::endl;
            return -1;
        }

        std::string rotKeyFileName = "/tmp/aegis/rotkey.txt";
        std::vector<int> galoisIdx;
        for (auto ind : keyInfos.asReader().getGaloisIndices()) {
            galoisIdx.push_back(ind);
        }
        if (galoisIdx.size() > 0) {
            std::ofstream rotationKeyFile(rotKeyFileName, std::ios::out | std::ios::binary);
            if (rotationKeyFile.is_open()) {
                if (!cryptoCtx->SerializeEvalAutomorphismKey(rotationKeyFile, SerType::BINARY)) {
                    std::cerr << "Error writing rotation keys" << std::endl;
                    return -1;
                }
                rotationKeyFile.close();
            }
            else {
                std::cerr << "Error serializing Rotation keys" << std::endl;
                return -1;
            }
        } else {
            rotKeyFileName = ""; //mean not has galois key
        }

        // call loadCryptoResources func
        if (!pRuntime->loadCryptoResources(ccFileName, pubKeyFileName, mulKeyFileName, rotKeyFileName)) {
            std::cout << "Test failure." << std::endl;
            return -1;
        }

        
        // case 1:
        {
            // scene 1:
            double a = 3.0;
            double b = 4.0;
            std::vector<double> expect_output(1, a*b);
            std::vector<double> output = encryptRunDecrypt_2(pRuntime, {a}, true, {b}, true);
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
            std::vector<double> output2 = encryptRunDecrypt_2(pRuntime, {a2}, true, {b2}, true);
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
            std::vector<double> output3 = encryptRunDecrypt_2(pRuntime, a3, true, b3, true);
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
            std::vector<double> v = {2.0, 2.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> expect_output = {6.0, 26.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> output = encryptRunDecrypt_2(pRuntime, m, true, v, true);
            for (auto i = 0; i < expect_output.size(); i++) {
                if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }

            // scene 2:
            std::vector<double> m2 = {2.0, 3.0, 0.0, 0.0, 5.0, 6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> v2 = {1.0, 0.0, 100.0, 200.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> expect_output2 = {2.0, 10.0, 100.0, 200.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> output2 = encryptRunDecrypt_2(pRuntime, m2, true, v2, true);
            for (auto i = 0; i < expect_output2.size(); i++) {
                if (!approximatelyEqual(output2[i], expect_output2[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }

            // scene 3:
            std::vector<double> m3 = {2.0, 3.0, 1.0, 1.0, 4.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> v3 = {1.0, 2.0, 3.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> expect_output3 = {8.0, 42.0, 3.0, 4.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            std::vector<double> output3 = encryptRunDecrypt_2(pRuntime, m3, true, v3, true);
            for (auto i = 0; i < expect_output3.size(); i++) {
                if (!approximatelyEqual(output3[i], expect_output3[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }
        }

        // case 3:
        {
            std::vector<double> m = {1.0};
            std::vector<double> v = {2.0};
            std::vector<double> expect_output = {17179869184.0};      
            std::vector<double> output = encryptRunDecrypt_2(pRuntime, m, true, v, true);
            for (auto i = 0; i < expect_output.size(); i++) {
                if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }

            std::vector<double> m2 = {1.0, 1.0};
            std::vector<double> v2 = {1.0, 2.0};
            std::vector<double> expect_output2 = {1.0, 17179869184.0};      
            std::vector<double> output2 = encryptRunDecrypt_2(pRuntime, m2, true, v2, true);
            for (auto i = 0; i < expect_output2.size(); i++) {
                if (!approximatelyEqual(output2[i], expect_output2[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }
        }

        // case 4:
        {
            std::vector<double> a1 = {10.0};
            std::vector<double> b1 = {14.0};
            std::vector<double> expect_output = {24.0};      
            std::vector<double> output = encryptRunDecrypt_2(pRuntime, a1, true, b1, true);
            for (auto i = 0; i < expect_output.size(); i++) {
                if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }
        }

        // case 5:
        {
            std::vector<double> a1 = {-2.0};
            std::vector<double> b1 = {-4.0};
            std::vector<double> expect_output = {-4.0};      
            std::vector<double> output = encryptRunDecrypt_2(pRuntime, a1, true, b1, true);
            for (auto i = 0; i < expect_output.size(); i++) {
                if (!approximatelyEqual(output[i], expect_output[i], 1e-6, 1e-6)) {
                    std::cout << "Test fail" << std::endl;
                    return -1;
                }
            }
        }

        std::cout << "Test pass." << std::endl;
        return 0;
    }
    */
}
