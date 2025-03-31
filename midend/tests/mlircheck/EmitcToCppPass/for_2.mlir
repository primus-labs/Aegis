// RUN: emitc-translate --mlir-to-cpp < %s | FileCheck %s

module {
  emitc.include <"vector">
  emitc.include "openfhe.h"
  emitc.verbatim "using namespace std;"
  emitc.verbatim "using namespace lbcrypto;"
  emitc.verbatim "using CiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using RLWECipher = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using LWECipher = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using PlaintextT = Plaintext;"
  emitc.verbatim "using Plain = Plaintext;"
  emitc.verbatim "using MutableCiphertextT = Ciphertext<DCRTPoly>;"
  emitc.verbatim "using CCParamsT = CCParams<CryptoContextCKKSRNS>;"
  emitc.verbatim "using CryptoContextT = CryptoContext<DCRTPoly>;"
  emitc.verbatim "using EvalKeyT = EvalKey<DCRTPoly>;"
  emitc.verbatim "using PrivateKeyT = PrivateKey<DCRTPoly>;"
  emitc.verbatim "using PublicKeyT = PublicKey<DCRTPoly>;"
  emitc.verbatim "#define Add(a, b) cryptoCtx->EvalAdd((a), (b))"
  emitc.verbatim "#define AddPlain(c, p) cryptoCtx->EvalAdd((c), (p))"
  emitc.verbatim "#define Mul(a, b) cryptoCtx->EvalMult((a), (b))"
  emitc.verbatim "#define MulPlain(c, p) cryptoCtx->EvalMult((c), (p))"
  emitc.verbatim "#define Rotate(c, idx) cryptoCtx->EvalRotate((c), (idx))"
  emitc.verbatim "#define MakePlain(...)  cryptoCtx->MakeCKKSPackedPlaintext(std::vector{VA_ARGS})"
  emitc.verbatim "#define Cast_Plain_To_Index(pt) pt->GetRealPackedValue()[0]"
  emitc.verbatim "#define Native_Load(v, idx) v[idx]"
  emitc.verbatim "CryptoContext<DCRTPoly> cryptoCtx;"
  emitc.verbatim "void init_cryptcontext() {"
  emitc.verbatim "   CCParams<CryptoContextBGVRNS> parameters;"
  emitc.verbatim "   //TODO"
  emitc.verbatim "   cryptoCtx = GenCryptoContext(parameters);"
  emitc.verbatim "   cryptoCtx->Enable(PKE);"
  emitc.verbatim "   cryptoCtx->Enable(KEYSWITCH);"
  emitc.verbatim "   cryptoCtx->Enable(LEVELEDSHE);"
  emitc.verbatim "}"
  func.func @MVP(%arg0: !emitc.opaque<"std::vector<Plain>"> {onnx.name = "input_x", onnx.type = "clear"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"RLWECipher"> {
    emitc.verbatim "init_cryptcontext();"
    %0 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(0.000000)">}> : () -> !emitc.opaque<"Plain">
    %1 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(1.000000)">}> : () -> !emitc.opaque<"Plain">
    %2 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(4.000000)">}> : () -> !emitc.opaque<"Plain">
    %3 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(5.000000)">}> : () -> !emitc.opaque<"Plain">
    %4 = emitc.call_opaque "Cast_Plain_To_Index"(%3) : (!emitc.opaque<"Plain">) -> index
    %5 = emitc.call_opaque "Cast_Plain_To_Index"(%2) : (!emitc.opaque<"Plain">) -> index
    %6 = emitc.call_opaque "Cast_Plain_To_Index"(%1) : (!emitc.opaque<"Plain">) -> index
    %7 = emitc.call_opaque "Cast_Plain_To_Index"(%0) : (!emitc.opaque<"Plain">) -> index
    %8 = emitc.call_opaque "Native_Load"(%arg0, %7) : (!emitc.opaque<"std::vector<Plain>">, index) -> !emitc.opaque<"Plain">
    %9 = emitc.call_opaque "MulPlain"(%arg1, %8) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %10 = emitc.call_opaque "Native_Load"(%arg0, %6) : (!emitc.opaque<"std::vector<Plain>">, index) -> !emitc.opaque<"Plain">
    %11 = emitc.call_opaque "MulPlain"(%arg1, %10) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %12 = emitc.call_opaque "Rotate"(%11) {args = [0 : index, 3 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %13 = emitc.call_opaque "Add"(%9, %12) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %14 = emitc.call_opaque "Load"(%13, %7) : (!emitc.opaque<"RLWECipher">, index) -> !emitc.opaque<"RLWECipher">
    %15 = emitc.call_opaque "Store"(%arg1, %14, %7) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">, index) -> !emitc.opaque<"RLWECipher">
    %16 = emitc.call_opaque "Native_Load"(%arg0, %5) : (!emitc.opaque<"std::vector<Plain>">, index) -> !emitc.opaque<"Plain">
    %17 = emitc.call_opaque "MulPlain"(%arg1, %16) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %18 = emitc.call_opaque "Native_Load"(%arg0, %4) : (!emitc.opaque<"std::vector<Plain>">, index) -> !emitc.opaque<"Plain">
    %19 = emitc.call_opaque "MulPlain"(%arg1, %18) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
    %20 = emitc.call_opaque "Rotate"(%17) {args = [0 : index, 1 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %21 = emitc.call_opaque "Add"(%20, %19) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %22 = emitc.call_opaque "Load"(%21, %6) : (!emitc.opaque<"RLWECipher">, index) -> !emitc.opaque<"RLWECipher">
    %23 = emitc.call_opaque "Store"(%arg1, %22, %6) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">, index) -> !emitc.opaque<"RLWECipher">
    return %arg1 : !emitc.opaque<"RLWECipher">
  }
}


// CHECK: init_cryptcontext();
// CHECK: Plain v3 = MakePlain(0.000000);
// CHECK: Plain v4 = MakePlain(1.000000);
// CHECK: Plain v5 = MakePlain(4.000000);
// CHECK: Plain v6 = MakePlain(5.000000);
// CHECK: size_t v7 = Cast_Plain_To_Index(v6);
// CHECK: size_t v8 = Cast_Plain_To_Index(v5);
// CHECK: size_t v9 = Cast_Plain_To_Index(v4);
// CHECK: size_t v10 = Cast_Plain_To_Index(v3);
// CHECK: Plain v11 = Native_Load(v1, v10);
// CHECK: RLWECipher v12 = MulPlain(v2, v11);
// CHECK: Plain v13 = Native_Load(v1, v9);
// CHECK: RLWECipher v14 = MulPlain(v2, v13);
// CHECK: RLWECipher v15 = Rotate(v14, 3);
// CHECK: RLWECipher v16 = Add(v12, v15);
// CHECK: Plain v19 = Native_Load(v1, v8);
// CHECK: RLWECipher v20 = MulPlain(v2, v19);
// CHECK: Plain v21 = Native_Load(v1, v7);
// CHECK: RLWECipher v22 = MulPlain(v2, v21);
// CHECK: RLWECipher v23 = Rotate(v20, 1);
// CHECK: RLWECipher v24 = Add(v23, v22);