// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module attributes {fhe.GaloisKeyIndex = dense<[3, 1, -1]> : vector<3xi32>} {
  emitc.include <"vector">
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
  emitc.verbatim "#define Native_Load(v, idx) v[idx]"
  emitc.verbatim "\0ACryptoContext<DCRTPoly> clientCC;\0APublicKey<DCRTPoly> clientPubKey;\0Aextern \22C\22\0Abool init_cryptcontext(const std::string &ccLoc, const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {\0A    clientCC->ClearEvalMultKeys();\0A    clientCC->ClearEvalAutomorphismKeys();\0A    lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();\0A    if (!Serial::DeserializeFromFile(ccLoc, clientCC, SerType::BINARY)) {\0A        std::cerr << \22Cannot read serialized data from: \22 << ccLoc << std::endl;\0A        return false;\0A    }\0A    if (!Serial::DeserializeFromFile(pubKeyLoc, clientPubKey, SerType::BINARY)) {\0A        std::cerr << \22I cannot read serialized data from: \22 << pubKeyLoc << std::endl;\0A        return false;\0A    }\0A    std::ifstream multKeyIStream(multKeyLoc, std::ios::in | std::ios::binary);\0A    if (!multKeyIStream.is_open()) {\0A        std::cerr << \22Cannot read serialization from \22 << multKeyLoc << std::endl;\0A        return false;\0A    }\0A    if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {\0A        std::cerr << \22Could not deserialize eval mult key file\22 << std::endl;\0A        return false;\0A    }\0A    if (!rotKeyLoc.empty()) {\0A        std::ifstream rotKeyIStream(rotKeyLoc, std::ios::in | std::ios::binary);\0A        if (!rotKeyIStream.is_open()) {\0A            std::cerr << \22Cannot read serialization from \22 << rotKeyLoc << std::endl;\0A            return false;\0A        }\0A        if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {\0A            std::cerr << \22Could not deserialize eval rot key file\22 << std::endl;\0A            return false;\0A        }\0A    }\0A    return true;\0A}\0A"
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
  emitc.verbatim "extern \22C\22"
  func.func @MVP(%arg0: !emitc.opaque<"PlainVector"> {onnx.name = "input_x", onnx.type = "clear"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"RLWECipher"> {
    %0 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(0,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    %1 = "emitc.constant"() <{value = #emitc.opaque<"MakeMultPlain(1,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %2 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(0)">}> : () -> !emitc.opaque<"Plain">
    %3 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(1)">}> : () -> !emitc.opaque<"Plain">
    %4 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(4)">}> : () -> !emitc.opaque<"Plain">
    %5 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(5)">}> : () -> !emitc.opaque<"Plain">
    %6 = emitc.call_opaque "Cast_Plain_To_Index"(%5) : (!emitc.opaque<"Plain">) -> index
    %7 = emitc.call_opaque "Cast_Plain_To_Index"(%4) : (!emitc.opaque<"Plain">) -> index
    %8 = emitc.call_opaque "Cast_Plain_To_Index"(%3) : (!emitc.opaque<"Plain">) -> index
    %9 = emitc.call_opaque "Cast_Plain_To_Index"(%2) : (!emitc.opaque<"Plain">) -> index
    %10 = emitc.call_opaque "Native_Load"(%arg0, %9) : (!emitc.opaque<"PlainVector">, index) -> !emitc.opaque<"Plain">
    %11 = emitc.call_opaque "MulPlain"(%arg1, %10) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %12 = emitc.call_opaque "Native_Load"(%arg0, %8) : (!emitc.opaque<"PlainVector">, index) -> !emitc.opaque<"Plain">
    %13 = emitc.call_opaque "MulPlain"(%arg1, %12) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %14 = emitc.call_opaque "Rotate"(%13) {args = [0 : index, 3 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %15 = emitc.call_opaque "Add"(%11, %14) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %16 = emitc.call_opaque "MulPlain"(%15, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %17 = emitc.call_opaque "MulPlain"(%arg1, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %18 = emitc.call_opaque "Add"(%17, %16) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%18, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %19 = emitc.call_opaque "Native_Load"(%arg0, %7) : (!emitc.opaque<"PlainVector">, index) -> !emitc.opaque<"Plain">
    %20 = emitc.call_opaque "MulPlain"(%arg1, %19) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %21 = emitc.call_opaque "Native_Load"(%arg0, %6) : (!emitc.opaque<"PlainVector">, index) -> !emitc.opaque<"Plain">
    %22 = emitc.call_opaque "MulPlain"(%arg1, %21) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %23 = emitc.call_opaque "Rotate"(%20) {args = [0 : index, 1 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %24 = emitc.call_opaque "Add"(%23, %22) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %25 = emitc.call_opaque "Rotate"(%24) {args = [0 : index, -1 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %26 = emitc.call_opaque "MulPlain"(%25, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %27 = emitc.call_opaque "Add"(%17, %26) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%27, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    return %arg1 : !emitc.opaque<"RLWECipher">
  }
}



//CHECK: extern "C"
//CHECK: bool init_cryptcontext(const std::string &ccLoc, const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {
//CHECK:     clientCC->ClearEvalMultKeys();
//CHECK:     clientCC->ClearEvalAutomorphismKeys();
//CHECK:     lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();
//CHECK:     if (!Serial::DeserializeFromFile(ccLoc, clientCC, SerType::BINARY)) {
//CHECK:         std::cerr << "Cannot read serialized data from: " << ccLoc << std::endl;
//CHECK:         return false;
//CHECK:     }
//CHECK:     if (!Serial::DeserializeFromFile(pubKeyLoc, clientPubKey, SerType::BINARY)) {
//CHECK:         std::cerr << "I cannot read serialized data from: " << pubKeyLoc << std::endl;
//CHECK:         return false;
//CHECK:     }
//CHECK:     std::ifstream multKeyIStream(multKeyLoc, std::ios::in | std::ios::binary);
//CHECK:     if (!multKeyIStream.is_open()) {
//CHECK:         std::cerr << "Cannot read serialization from " << multKeyLoc << std::endl;
//CHECK:         return false;
//CHECK:     }
//CHECK:     if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {
//CHECK:         std::cerr << "Could not deserialize eval mult key file" << std::endl;
//CHECK:         return false;
//CHECK:     }
//CHECK:     if (!rotKeyLoc.empty()) {
//CHECK:         std::ifstream rotKeyIStream(rotKeyLoc, std::ios::in | std::ios::binary);
//CHECK:         if (!rotKeyIStream.is_open()) {
//CHECK:             std::cerr << "Cannot read serialization from " << rotKeyLoc << std::endl;
//CHECK:             return false;
//CHECK:         }
//CHECK:         if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {
//CHECK:             std::cerr << "Could not deserialize eval rot key file" << std::endl;
//CHECK:             return false;
//CHECK:         }
//CHECK:     }
//CHECK:     return true;
//CHECK: }