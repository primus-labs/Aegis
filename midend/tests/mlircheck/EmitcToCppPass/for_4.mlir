// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module attributes {fhe.GaloisKeyIndex = dense<[-15, -4, -13, -12]> : vector<4xi32>} {
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
  emitc.verbatim "#define MakePlain(a)  double(a)"
  emitc.verbatim "#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}"
  emitc.verbatim "#define Cast_Plain_To_Index(clr) size_t(clr)"
  emitc.verbatim "#define LoadPlainWithIndex(v, idx) v[idx]"
  emitc.verbatim "\0ACryptoContext<DCRTPoly> clientCC;\0APublicKey<DCRTPoly> clientPubKey;\0Aextern \22C\22\0Abool init_cryptcontext(const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {\0A    CCParams<CryptoContextCKKSRNS> parameters;\0A    parameters.SetMultiplicativeDepth(8);\0A    parameters.SetFirstModSize(60);\0A    parameters.SetScalingModSize(50);\0A    parameters.SetBatchSize(2048);\0A    clientCC = GenCryptoContext(parameters);\0A    clientCC->ClearEvalMultKeys();\0A    clientCC->ClearEvalAutomorphismKeys();\0A    if (!Serial::DeserializeFromFile(pubKeyLoc, clientPubKey, SerType::BINARY)) {\0A        std::cerr << \22Cannot read serialized data from: \22 << pubKeyLoc << std::endl;\0A        return false;\0A    }\0A    std::ifstream multKeyIStream(multKeyLoc, std::ios::in | std::ios::binary);\0A    if (!multKeyIStream.is_open()) {\0A        std::cerr << \22Cannot read serialization from \22 << multKeyLoc << std::endl;\0A        return false;\0A    }\0A    if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {\0A        std::cerr << \22Could not deserialize eval mult key file\22 << std::endl;\0A        return false;\0A    }\0A    if (!rotKeyLoc.empty()) {\0A        std::ifstream rotKeyIStream(rotKeyLoc, std::ios::in | std::ios::binary);\0A        if (!rotKeyIStream.is_open()) {\0A            std::cerr << \22Cannot read serialization from \22 << rotKeyLoc << std::endl;\0A            return false;\0A        }\0A        if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {\0A            std::cerr << \22Could not deserialize eval rot key file\22 << std::endl;\0A            return false;\0A        }\0A    }\0A    return true;\0A}\0A"
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
  func.func @MVP(%arg0: !emitc.opaque<"RLWECipher"> {onnx.dims = [16], onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.dims = [4], onnx.name = "input_y", onnx.type = "encrypted"}) -> (!emitc.opaque<"RLWECipher"> {onnx.dims = [4]}) {
    %0 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %1 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %2 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %3 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %4 = emitc.call_opaque "Mul"(%arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %5 = emitc.call_opaque "Rotate"(%4) {args = [0 : index, -15 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %6 = emitc.call_opaque "Add"(%4, %5) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %7 = emitc.call_opaque "MulPlain"(%6, %3) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %8 = emitc.call_opaque "MulPlain"(%arg1, %2) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %9 = emitc.call_opaque "Add"(%8, %7) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%9, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %10 = emitc.call_opaque "Rotate"(%arg1) {args = [0 : index, -4 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %11 = emitc.call_opaque "Mul"(%arg0, %10) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %12 = emitc.call_opaque "Rotate"(%arg1) {args = [0 : index, -4 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %13 = emitc.call_opaque "Mul"(%arg0, %12) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %14 = emitc.call_opaque "Rotate"(%11) {args = [0 : index, -13 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %15 = emitc.call_opaque "Rotate"(%13) {args = [0 : index, -12 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %16 = emitc.call_opaque "Add"(%14, %15) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %17 = emitc.call_opaque "MulPlain"(%16, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %18 = emitc.call_opaque "MulPlain"(%arg1, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %19 = emitc.call_opaque "Add"(%18, %17) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%19, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    return %arg1 : !emitc.opaque<"RLWECipher">
  }
  emitc.verbatim "\0Aextern \22C\22 \0Astd::vector<uint8_t> aegis_mlir_MVP(const std::vector<uint8_t> &buf1, const std::vector<uint8_t> &buf2) {\0A    \0A    Ciphertext<DCRTPoly> v1;\0A    std::stringstream ss1;\0A    ss1.write(reinterpret_cast<const char *>(buf1.data()), buf1.size());\0A    Serial::Deserialize(v1, ss1, SerType::BINARY);\0A\0A    Ciphertext<DCRTPoly> v2;\0A    std::stringstream ss2;\0A    ss2.write(reinterpret_cast<const char *>(buf2.data()), buf2.size());\0A    Serial::Deserialize(v2, ss2, SerType::BINARY);\0A\0A\0A    RLWECipher retV = MVP(v1, v2);\0A\0A    std::stringstream retss;\0A    Serial::Serialize(retV, retss, SerType::BINARY);\0A    std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());\0A    return retBuf;\0A}\0A"
}


//CHECK: extern "C" 
//CHECK: std::vector<uint8_t> aegis_mlir_MVP(const std::vector<uint8_t> &buf1, const std::vector<uint8_t> &buf2) {
//CHECK:     Ciphertext<DCRTPoly> v1;
//CHECK:     std::stringstream ss1;
//CHECK:     ss1.write(reinterpret_cast<const char *>(buf1.data()), buf1.size());
//CHECK:     Serial::Deserialize(v1, ss1, SerType::BINARY);
//CHECK:     Ciphertext<DCRTPoly> v2;
//CHECK:     std::stringstream ss2;
//CHECK:     ss2.write(reinterpret_cast<const char *>(buf2.data()), buf2.size());
//CHECK:     Serial::Deserialize(v2, ss2, SerType::BINARY);
//CHECK:     RLWECipher retV = MVP(v1, v2);
//CHECK:     std::stringstream retss;
//CHECK:     Serial::Serialize(retV, retss, SerType::BINARY);
//CHECK:     std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
//CHECK:     return retBuf;
//CHECK: }
