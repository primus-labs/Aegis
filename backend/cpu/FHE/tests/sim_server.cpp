// simulate the server
// load pub keys and context, ciphertexts
// do some computes
#include "Common/Protocol.h"
#include "CryptoContextMgr.h"
#include "FheKey.h"
#include "KeysetGenerator.h"
#include "MockKeyInfo.h"
#include "Operate.h"
#include "utils.h"
#include <protocol.capnp.h>

using namespace aegiscpu;
using namespace openfhe;

#include <cstdint>
#include <iostream>
#include <string>
using namespace std;

void test() {
  Ciphertext<DCRTPoly> c1;
  Ciphertext<DCRTPoly> c2;
  LoadFromFile("c1.bin", c1);
  LoadFromFile("c2.bin", c2);

  auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  {
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
    auto cRot1 = cc->EvalRotate(c1, 1);
    auto cRot2 = cc->EvalRotate(c1, -2);

    // dump to file
    DumpToFile("cAdd.bin", cAdd);
    DumpToFile("cSub.bin", cSub);
    DumpToFile("cScalar.bin", cScalar);
    DumpToFile("cMul.bin", cMul);
    DumpToFile("cRot1.bin", cRot1);
    DumpToFile("cRot2.bin", cRot2);
  }
}

int main(int argc, char *argv[]) {
  std::string pub_keys;
  fromFile("./pub_keys.bin", pub_keys);

  auto &keyset = FheKeyset::getInstance();
  keyset.loads(pub_keys);

  test();
  return 0;
}