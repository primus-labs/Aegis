// simulate the client(1/2)
// generate keys and context and dump them
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
  auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  auto pub_key = FheKeyset::getInstance().getPubKey()->getKey();

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

  // dump to file
  DumpToFile("c1.bin", c1);
  DumpToFile("c2.bin", c2);

  auto &keyset = FheKeyset::getInstance();
  // publicly keys(and context) for server
  auto pub_keys = keyset.dumps(false);
  toFile("./pub_keys.bin", pub_keys);

  // all keys(and context) for client self
  auto all_keys = keyset.dumps();
  toFile("./all_keys.bin", all_keys);
}

int main(int argc, char *argv[]) {
  ProtoMessage<aegisprotocol::KeyInfo> keyInfo;
  mockKeyInfo01(keyInfo);

  // initialize
  KeysetGenerator::generate(keyInfo);

  test();
  return 0;
}