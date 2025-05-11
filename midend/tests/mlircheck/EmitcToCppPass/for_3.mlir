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
  emitc.verbatim "#define Sub(a, b) cryptoCtx->EvalSub((a), (b))"
  emitc.verbatim "#define SubPlain(c, p) cryptoCtx->EvalSub((c), (p))"
  emitc.verbatim "#define Mul(a, b) cryptoCtx->EvalMult((a), (b))"
  emitc.verbatim "#define MulPlain(c, p) cryptoCtx->EvalMult((c), (p))"
  emitc.verbatim "#define Rotate(c, idx) cryptoCtx->EvalRotate((c), (idx))"
  emitc.verbatim "#define MakePlain(...)  cryptoCtx->MakeCKKSPackedPlaintext(std::vector<double>{__VA_ARGS__})"
  emitc.verbatim "#define Cast_Plain_To_Index(pt) pt->GetRealPackedValue()[0]"
  emitc.verbatim "#define LoadPlainWithIndex(v, idx) v[idx]"
  emitc.verbatim "CryptoContext<DCRTPoly> cryptoCtx;"
  emitc.verbatim "void init_cryptcontext() {"
  emitc.verbatim "   CCParams<CryptoContextCKKSRNS> parameters;"
  emitc.verbatim "   parameters.SetMultiplicativeDepth(8);"
  emitc.verbatim "   parameters.SetFirstModSize(60);"
  emitc.verbatim "   parameters.SetScalingModSize(50);"
  emitc.verbatim "   parameters.SetBatchSize(2048);"
  emitc.verbatim "   cryptoCtx = GenCryptoContext(parameters);"
  emitc.verbatim "   cryptoCtx->Enable(PKE);"
  emitc.verbatim "   cryptoCtx->Enable(KEYSWITCH);"
  emitc.verbatim "   cryptoCtx->Enable(LEVELEDSHE);"
  emitc.verbatim "}"
  emitc.verbatim "extern \22C\22"
  func.func @MVP(%arg0: !emitc.opaque<"RLWECipher"> {onnx.name = "input_x", onnx.type = "encrypted"}, %arg1: !emitc.opaque<"RLWECipher"> {onnx.name = "input_y", onnx.type = "encrypted"}) -> !emitc.opaque<"RLWECipher"> {
    %0 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)">}> : () -> !emitc.opaque<"PlainVector">
    %1 = "emitc.constant"() <{value = #emitc.opaque<"MakePlain(0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1)">}> : () -> !emitc.opaque<"PlainVector">
    emitc.verbatim "init_cryptcontext();"
    %2 = emitc.call_opaque "Mul"(%arg0, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %3 = emitc.call_opaque "Rotate"(%2) {args = [0 : index, 15 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %4 = emitc.call_opaque "Add"(%2, %3) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %5 = emitc.call_opaque "MulPlain"(%4, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %6 = emitc.call_opaque "MulPlain"(%arg1, %1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %7 = emitc.call_opaque "Add"(%6, %5) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%7, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    %8 = emitc.call_opaque "Rotate"(%arg1) {args = [0 : index, 4 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %9 = emitc.call_opaque "Mul"(%arg0, %8) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %10 = emitc.call_opaque "Rotate"(%arg1) {args = [0 : index, 4 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %11 = emitc.call_opaque "Mul"(%arg0, %10) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %12 = emitc.call_opaque "Rotate"(%9) {args = [0 : index, 13 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %13 = emitc.call_opaque "Rotate"(%11) {args = [0 : index, 12 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %14 = emitc.call_opaque "Add"(%12, %13) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %15 = emitc.call_opaque "Rotate"(%14) {args = [0 : index, -1 : si32]} : (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    %16 = emitc.call_opaque "MulPlain"(%15, %0) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"PlainVector">) -> !emitc.opaque<"RLWECipher">
    %17 = emitc.call_opaque "Add"(%6, %16) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
    emitc.call_opaque "Copy"(%17, %arg1) : (!emitc.opaque<"RLWECipher">, !emitc.opaque<"RLWECipher">) -> ()
    return %arg1 : !emitc.opaque<"RLWECipher">
  }
}


//CHECK: RLWECipher MVP(RLWECipher v1, RLWECipher v2) {
//CHECK:   PlainVector v3 = MakePlain(1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
//CHECK:   PlainVector v4 = MakePlain(0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);
//CHECK:   init_cryptcontext();
//CHECK:   RLWECipher v5 = Mul(v1, v2);
//CHECK:   RLWECipher v6 = Rotate(v5, 15);
//CHECK:   RLWECipher v7 = Add(v5, v6);
//CHECK:   RLWECipher v8 = MulPlain(v7, v3);
//CHECK:   RLWECipher v9 = MulPlain(v2, v4);
//CHECK:   RLWECipher v10 = Add(v9, v8);
//CHECK:   Copy(v10, v2);
//CHECK:   RLWECipher v11 = Rotate(v2, 4);
//CHECK:   RLWECipher v12 = Mul(v1, v11);
//CHECK:   RLWECipher v13 = Rotate(v2, 4);
//CHECK:   RLWECipher v14 = Mul(v1, v13);
//CHECK:   RLWECipher v15 = Rotate(v12, 13);
//CHECK:   RLWECipher v16 = Rotate(v14, 12);
//CHECK:   RLWECipher v17 = Add(v15, v16);
//CHECK:   RLWECipher v18 = Rotate(v17, -1);
//CHECK:   RLWECipher v19 = MulPlain(v18, v3);
//CHECK:   RLWECipher v20 = Add(v9, v19);
//CHECK:   Copy(v20, v2);
//CHECK:   return v2;
//CHECK: }
