// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module {
  emitc.include <"vector">
  emitc.include <"iostream">
  emitc.include "openfhe.h"
  emitc.verbatim "using namespace std;"
  emitc.verbatim "using namespace lbcrypto;"
  emitc.verbatim "using CiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using RLWECipher = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using LWECipher = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using PlaintextT = Plaintext;"
  emitc.verbatim "using Plain = double;"
  emitc.verbatim "using PlainVector = std::vector<double>;"
  emitc.verbatim "using PlainMatrix = std::vector<PlainVector>;"
  emitc.verbatim "using MutableCiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using CCParamsT = CCParams<CryptoContextCKKSRNS>;"
  emitc.verbatim "using CryptoContextT = CryptoContext<DCRTPoly>;"
  emitc.verbatim "using EvalKeyT = EvalKey<DCRTPoly>;"
  emitc.verbatim "using PrivateKeyT = PrivateKey<DCRTPoly>;"
  emitc.verbatim "using PublicKeyT = PublicKey<DCRTPoly>;"
  emitc.verbatim "#define Copy(src, dest) dest = src"
  emitc.verbatim "#define Add(a, b) clientCC->EvalAdd((a), (b))"
  emitc.verbatim "#define AddPlain(c, p) AddPlainImpl((c), (p))"
  emitc.verbatim "#define Sub(a, b) clientCC->EvalSub((a), (b))"
  emitc.verbatim "#define SubPlain(c, p) SubPlainImpl((c), (p))"
  emitc.verbatim "#define Mul(a, b) clientCC->EvalMult((a), (b))"
  emitc.verbatim "#define MulPlain(c, p) MulPlainImpl((c), (p))"
  emitc.verbatim "#define Rotate(c, idx) clientCC->EvalRotate((c), (idx))"
  emitc.verbatim "#define Bootstrap(a) clientCC->EvalBootstrap(a)"
  emitc.verbatim "#define MakePlain(a)  double(a)"
  emitc.verbatim "#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}"
  emitc.verbatim "#define Cast_Plain_To_Index(clr) size_t(clr)"
  emitc.verbatim "#define Native_Load(v, idx) v[idx]"
  emitc.verbatim "\0ACryptoContext<DCRTPoly> clientCC;\0APublicKey<DCRTPoly> clientPubKey;\0Aextern \22C\22\0Abool init_cryptcontext(const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {\0A    std::vector<uint32_t> levelBudget = {3, 1};\0A    unsigned mulDepth = 8;\0A    if (1) {\0A        SecretKeyDist secretKeyDist = UNIFORM_TERNARY;\0A        mulDepth += FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist);\0A    }\0A    CCParams<CryptoContextCKKSRNS> parameters;\0A    parameters.SetMultiplicativeDepth(mulDepth);\0A    parameters.SetFirstModSize(60);\0A    parameters.SetScalingModSize(50);\0A    parameters.SetBatchSize(16);\0A    clientCC = GenCryptoContext(parameters);\0A    clientCC->ClearEvalMultKeys();\0A    clientCC->ClearEvalAutomorphismKeys();\0A    if (!Serial::DeserializeFromFile(pubKeyLoc, clientPubKey, SerType::BINARY)) {\0A        std::cerr << \22Cannot read serialized data from: \22 << pubKeyLoc << std::endl;\0A        return false;\0A    }\0A    std::ifstream multKeyIStream(multKeyLoc, std::ios::in | std::ios::binary);\0A    if (!multKeyIStream.is_open()) {\0A        std::cerr << \22Cannot read serialization from \22 << multKeyLoc << std::endl;\0A        return false;\0A    }\0A    if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {\0A        std::cerr << \22Could not deserialize eval mult key file\22 << std::endl;\0A        return false;\0A    }\0A    if (!rotKeyLoc.empty()) {\0A        std::ifstream rotKeyIStream(rotKeyLoc, std::ios::in | std::ios::binary);\0A        if (!rotKeyIStream.is_open()) {\0A            std::cerr << \22Cannot read serialization from \22 << rotKeyLoc << std::endl;\0A            return false;\0A        }\0A        if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {\0A            std::cerr << \22Could not deserialize eval rot key file\22 << std::endl;\0A            return false;\0A        }\0A    }\0A    return true;\0A}\0A"
  emitc.verbatim "inline RLWECipher AddPlainImpl(RLWECipher a, Plain b) {"
  emitc.verbatim "    return clientCC->EvalAdd(a, b);"
  emitc.verbatim "}"
  emitc.verbatim "inline RLWECipher AddPlainImpl(RLWECipher a, PlainVector b) {"
  emitc.verbatim "    return clientCC->EvalAdd(a, clientCC->MakeCKKSPackedPlaintext(b));"
  emitc.verbatim "}"
  emitc.verbatim "inline RLWECipher SubPlainImpl(RLWECipher a, Plain b) {"
  emitc.verbatim "    return clientCC->EvalSub(a, b);"
  emitc.verbatim "}"
  emitc.verbatim "inline RLWECipher SubPlainImpl(RLWECipher a, PlainVector b) {"
  emitc.verbatim "    return clientCC->EvalSub(a, clientCC->MakeCKKSPackedPlaintext(b));"
  emitc.verbatim "}"
  emitc.verbatim "inline RLWECipher MulPlainImpl(RLWECipher a, Plain b) {"
  emitc.verbatim "    return clientCC->EvalMult(a, b);"
  emitc.verbatim "}"
  emitc.verbatim "inline RLWECipher MulPlainImpl(RLWECipher a, PlainVector b) {"
  emitc.verbatim "    return clientCC->EvalMult(a, clientCC->MakeCKKSPackedPlaintext(b));"
  emitc.verbatim "}"
  func.func @main_graph(%arg0: !emitc.opaque<"RLWECipher"> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"RLWECipher"> {
    %0 = emitc.call_opaque "Mul"(%arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %1 = emitc.call_opaque "Mul"(%0, %arg0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %2 = emitc.call_opaque "Mul"(%0, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %3 = emitc.call_opaque "Mul"(%1, %2) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %4 = emitc.call_opaque "Mul"(%2, %3) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %5 = emitc.call_opaque "Mul"(%3, %4) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %6 = emitc.call_opaque "Mul"(%4, %5) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %7 = emitc.call_opaque "Bootstrap"(%6) : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %8 = emitc.call_opaque "Mul"(%5, %7) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %9 = emitc.call_opaque "Bootstrap"(%8) : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %10 = emitc.call_opaque "Mul"(%7, %9) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    return %10 : !emitc.opaque<"RLWECipher">
  }
  emitc.verbatim "\0Aextern \22C\22 \0Astd::vector<uint8_t> aegis_mlir_MVP(const std::vector<uint8_t> &buf1, const std::vector<uint8_t> &buf2) {\0A    \0A    Ciphertext<DCRTPoly> v1;\0A    std::stringstream ss1;\0A    ss1.write(reinterpret_cast<const char *>(buf1.data()), buf1.size());\0A    Serial::Deserialize(v1, ss1, SerType::BINARY);\0A\0A    Ciphertext<DCRTPoly> v2;\0A    std::stringstream ss2;\0A    ss2.write(reinterpret_cast<const char *>(buf2.data()), buf2.size());\0A    Serial::Deserialize(v2, ss2, SerType::BINARY);\0A\0A    RLWECipher retV = MVP(v1, v2);\0A    std::stringstream retss;\0A    Serial::Serialize(retV, retss, SerType::BINARY);\0A    std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());\0A    return retBuf;\0A}\0A"
}


//CHECK: RLWECipher main_graph(RLWECipher v1, RLWECipher v2) {
//CHECK:   RLWECipher v3 = Mul(v1, v2);
//CHECK:   RLWECipher v4 = Mul(v3, v1);
//CHECK:   RLWECipher v5 = Mul(v3, v4);
//CHECK:   RLWECipher v6 = Mul(v4, v5);
//CHECK:   RLWECipher v7 = Mul(v5, v6);
//CHECK:   RLWECipher v8 = Mul(v6, v7);
//CHECK:   RLWECipher v9 = Mul(v7, v8);
//CHECK:   RLWECipher v10 = Bootstrap(v9);
//CHECK:   RLWECipher v11 = Mul(v8, v10);
//CHECK:   RLWECipher v12 = Bootstrap(v11);
//CHECK:   RLWECipher v13 = Mul(v10, v12);
//CHECK:   return v13;
//CHECK: }


