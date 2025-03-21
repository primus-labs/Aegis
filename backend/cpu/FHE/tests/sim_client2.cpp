// simulate the client(2/2)
// load all keys and context, ciphertexts
// do some computes, decrypt
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

void test() {
  Ciphertext<DCRTPoly> cAdd, cSub, cScalar, cMul, cRot1, cRot2;
  LoadFromFile("cAdd.bin", cAdd);
  LoadFromFile("cSub.bin", cSub);
  LoadFromFile("cScalar.bin", cScalar);
  LoadFromFile("cMul.bin", cMul);
  LoadFromFile("cRot1.bin", cRot1);
  LoadFromFile("cRot2.bin", cRot2);

  auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  auto pri_key = FheKeyset::getInstance().getPriKey()->getKey();
  {

    // Step 5: Decryption and output
    Plaintext result;

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

int main(int argc, char *argv[]) {
  std::string all_keys;
  fromFile("./all_keys.bin", all_keys);

  auto &keyset = FheKeyset::getInstance();
  keyset.loads(all_keys);

  test();
  return 0;
}