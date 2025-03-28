#include "Common/Protocol.h"
#include "CryptoContextMgr.h"
#include "FheKey.h"
#include "KeysetGenerator.h"
#include "MockKeyInfo.h"
#include "Operate.h"
#include "utils.h"
#include <protocol.capnp.h>

using namespace aegiscpu;

#include <cstdint>
#include <iostream>
#include <string>
using namespace std;

void _test_real_number() {
  cout << "=== === real number" << endl;

  auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  auto pub_key = FheKeyset::getInstance().getPubKey()->getKey();
  auto pri_key = FheKeyset::getInstance().getPriKey()->getKey();

  {
    // ref: (openfhe) src/pke/examples/simple-real-numbers.cpp

    // Inputs
    std::vector<double> x1 = {0.25, 0.5, 0.75, 1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> x2 = {5.0, 4.0, 3.0, 2.0, 1.0, 0.75, 0.5, 0.25};

    // Encoding as plaintexts
    Plaintext ptxt1 = cc->MakeCKKSPackedPlaintext(x1);
    Plaintext ptxt2 = cc->MakeCKKSPackedPlaintext(x2);

    std::cout << "Input x1: " << ptxt1 << std::endl;
    std::cout << "Input x2: " << ptxt2 << std::endl;

    // Encrypt the encoded vectors
    auto c1 = cc->Encrypt(pub_key, ptxt1);
    auto c2 = cc->Encrypt(pub_key, ptxt2);

    // Evaluation

    // Homomorphic addition
    auto cAdd = cc->EvalAdd(c1, c2);

    // Homomorphic subtraction
    auto cSub = cc->EvalSub(c1, c2);

    // Homomorphic scalar multiplication
    auto cScalar = cc->EvalMult(c1, 4.0);

    // Homomorphic multiplication
    auto cMul = cc->EvalMult(c1, c2);

    // Homomorphic rotations
    // cc->EvalRotateKeyGen(pri_key, {1, -2});
    auto cRot1 = cc->EvalRotate(c1, 1);
    auto cRot2 = cc->EvalRotate(c1, -2);

    // Step 5: Decryption and output
    Plaintext result;
    cc->Decrypt(pri_key, c1, &result);
    std::cout << "x1 = " << result;
    std::cout << "Estimated precision in bits: " << result->GetLogPrecision()
              << std::endl;

    // Decrypt the result of addition
    cc->Decrypt(pri_key, cAdd, &result);
    std::cout << "x1 + x2 = " << result;
    std::cout << "Estimated precision in bits: " << result->GetLogPrecision()
              << std::endl;

    // Decrypt the result of subtraction
    cc->Decrypt(pri_key, cSub, &result);
    std::cout << "x1 - x2 = " << result << std::endl;

    // Decrypt the result of scalar multiplication
    cc->Decrypt(pri_key, cScalar, &result);
    std::cout << "4 * x1 = " << result << std::endl;

    // Decrypt the result of multiplication
    cc->Decrypt(pri_key, cMul, &result);
    std::cout << "x1 * x2 = " << result << std::endl;

    // Decrypt the result of rotations

    cc->Decrypt(pri_key, cRot1, &result);
    std::cout
        << std::endl
        << "In rotations, very small outputs (~10^-10 here) correspond to 0's:"
        << std::endl;
    std::cout << "x1 rotate by 1 = " << result << std::endl;

    cc->Decrypt(pri_key, cRot2, &result);
    std::cout << "x1 rotate by -2 = " << result << std::endl;
  }
}

void _test_encrypt() {
  cout << "=== === encrypt/decrypt" << endl;

  std::vector<double> x1 = {0.25, 0.5, 0.75, 1.0, 2.0, 3.0, 4.0, 5.0};
  std::vector<double> x2 = {5.0, 4.0, 3.0, 2.0, 1.0, 0.75, 0.5, 0.25};
  auto c1 = aegiscpu::encrypt(x1);
  auto c2 = aegiscpu::encrypt(x2);
  {
    // test decrypt
    // cout << "x1:" << x1 << endl;
    auto d1 = aegiscpu::decrypt(c1, x1.size());
    // PrintDiff(x1, d1);
    // cout << "d1:" << d1 << endl;
    assert_eq(x1, d1, "decrypt on c1, !=x1");

    // cout << "x2:" << x2 << endl;
    auto d2 = aegiscpu::decrypt(c2, x2.size());
    // PrintDiff(x2, d2);
    // cout << "d2:" << d2 << endl;
    assert_eq(x2, d2, "decrypt on c2, !=x2");
  }

  {
    std::vector<double> x1 = RandomVector(8, -10.0, 10.0);
    auto c1 = aegiscpu::encrypt(x1);
    auto d1 = aegiscpu::decrypt(c1, x1.size());
    assert_eq(x1, d1, "decrypt on c1, !=x1");
  }
}

void _test_serialize() {
  cout << "=== === serialize/deserialize" << endl;
  //

  auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  auto pub_key = FheKeyset::getInstance().getPubKey()->getKey();
  auto pri_key = FheKeyset::getInstance().getPriKey()->getKey();
  FHEPrivateKey fhePriKey(pri_key);
  FHEPublicKey fhePubKey(pub_key);

  std::vector<double> x1 = {0.25, 0.5, 0.75, 1.0, 2.0, 3.0, 4.0, 5.0};
  Plaintext ptxt1 = cc->MakeCKKSPackedPlaintext(x1);
  auto c1 = cc->Encrypt(pub_key, ptxt1);
  Plaintext result;

  cout << "=== === serialize/deserialize private key" << endl;
  auto ser_sk = fhePriKey.serialize();
  {
    cc->Decrypt(fhePriKey.getKey(), c1, &result);
    auto d1 = result->GetRealPackedValue();
    assert_eq(x1, d1, "decrypt on c1, !=x1");
  }
  {
    fhePriKey.deserialize(ser_sk);
    cc->Decrypt(fhePriKey.getKey(), c1, &result);
    auto d1 = result->GetRealPackedValue();
    assert_eq(x1, d1, "decrypt on c1, !=x1");
  }

  cout << "=== === serialize/deserialize public key" << endl;
  auto ser_pk = fhePubKey.serialize();
  {
    auto c1 = cc->Encrypt(fhePubKey.getKey(), ptxt1);
    cc->Decrypt(fhePriKey.getKey(), c1, &result);
    auto d1 = result->GetRealPackedValue();
    assert_eq(x1, d1, "decrypt on c1, !=x1");
  }
  {
    fhePubKey.deserialize(ser_pk);
    auto c1 = cc->Encrypt(fhePubKey.getKey(), ptxt1);
    cc->Decrypt(pri_key, c1, &result);
    auto d1 = result->GetRealPackedValue();
    assert_eq(x1, d1, "decrypt on c1, !=x1");
  }

  cout << "=== === serialize/deserialize context" << endl;
  {
    std::stringstream ss;
    Serial::Serialize(cc, ss, SerType::BINARY);
    auto ser_cc = ss.str();
  }
  cout << "=== === serialize/deserialize eval key" << endl;
  {
    std::stringstream ss;
    if (!cc->SerializeEvalMultKey(ss, SerType::BINARY)) {
      cout << "serialize eval mult keys failed" << endl;
    }
    auto ser_mult = ss.str();
  }
  {
    std::stringstream ss;
    if (!cc->SerializeEvalAutomorphismKey(ss, SerType::BINARY)) {
      cout << "serialize eval rotate/bootstrap keys failed" << endl;
    }
    auto ser_automorphism = ss.str();
  }
}
void _test_dumps() {
  cout << "=== === dumps/loads" << endl;
  //
  auto &keyset = FheKeyset::getInstance();
  auto keys = keyset.dumps();
  keyset.loads(keys);
}

void test_01() {
  ProtoMessage<aegisprotocol::KeyInfo> keyInfo;
  mockKeyInfo01(keyInfo);

  // initialize
  KeysetGenerator::generate(keyInfo);

  // tests
  _test_real_number();
  _test_encrypt();
  _test_serialize();
  _test_dumps();
  _test_real_number();
}

int main(int argc, char *argv[]) {
  cout << "=== backent.cpu.FHE.tests ===" << endl;

  test_01();

  return 0;
}