#include "Operate.h"
#include "CryptoContextMgr.h"
#include "FheKeyset.h"

namespace aegiscpu {

std::vector<uint8_t> encrypt(std::vector<double> &data) {
  CryptoContext<DCRTPoly> cc =
      CryptoContextMgr::getInstance().getCryptoContext();

  Plaintext ptValue = cc->MakeCKKSPackedPlaintext(data);

  PublicKey<DCRTPoly> pk = FheKeyset::getInstance().getPubKey()->getKey();
  Ciphertext<DCRTPoly> ctValue = cc->Encrypt(pk, ptValue);

  std::stringstream ss;
  Serial::Serialize(ctValue, ss, SerType::BINARY);
  std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ss)),
                              std::istreambuf_iterator<char>());

  return buffer;
}

std::vector<double> decrypt(std::vector<uint8_t> &data) {
  CryptoContext<DCRTPoly> cc =
      CryptoContextMgr::getInstance().getCryptoContext();

  // Deserialize output byte array into a ciphertext object.
  std::stringstream ss;
  ss.write(reinterpret_cast<char *>(data.data()), data.size());
  Ciphertext<DCRTPoly> deserCiphertext;
  Serial::Deserialize(deserCiphertext, ss, SerType::BINARY);

  // get private key
  PrivateKey<DCRTPoly> priKey = FheKeyset::getInstance().getPriKey()->getKey();

  // decrypt
  Plaintext ptValue;
  cc->Decrypt(priKey, deserCiphertext, &ptValue);
  // TODO: Need know plaintext size.
  // ptValue->SetLength(data.size() / sizeof(double)); // invalid size

  return ptValue->GetRealPackedValue();
}

} // namespace aegiscpu