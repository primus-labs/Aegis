#include "FheKeyset.h"
#include "CryptoContextMgr.h"
#include "Operate.h"

#include <iostream>
#include <string>
using namespace std;

// NOTE!!! MUST INCLUDE THE FOLLOWING HEADERS
// header files needed for serialization
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"
using namespace lbcrypto;

namespace aegiscpu {
namespace openfhe {

static std::string serializeBE(uint32_t value) {
  uint8_t buffer[4] = {0};
  buffer[3] = value & 0xFF;
  buffer[2] = (value >> 8) & 0xFF;
  buffer[1] = (value >> 16) & 0xFF;
  buffer[0] = (value >> 24) & 0xFF;
  return std::string((char *)buffer, 4);
}

static uint32_t deserializeBE(const uint8_t buffer[4]) {
  return (uint32_t(buffer[3]) | (uint32_t(buffer[2]) << 8) |
          (uint32_t(buffer[1]) << 16) | (uint32_t(buffer[0]) << 24));
}

enum class SerialTag {
  CryptoContext = 1,
  EvalMultKey = 2,
  EvalAutomorphismKey = 3,
  PublicKey = 4,
  PrivateKey = 5,
};

// todo: optimize
static constexpr SerialTag ToSerialTag(uint32_t value) {
  switch (value) {
  case 1:
    return SerialTag::CryptoContext;
  case 2:
    return SerialTag::EvalMultKey;
  case 3:
    return SerialTag::EvalAutomorphismKey;
  case 4:
    return SerialTag::PublicKey;
  case 5:
    return SerialTag::PrivateKey;
  default:
    throw std::invalid_argument("Invalid SerialTag value");
  }
}

static void serialize(SerialTag tag, std::ostream &os,
                      const std::string &data) {
  os << serializeBE(static_cast<uint32_t>(tag)) << serializeBE(data.size())
     << data;
}

/**
 * format: TAG(4)|SIZE(4)|CONTENT(SIZE)
 * note: Serialize CryptoContext first
 */
std::string FheKeyset::dumps(bool contain_sk) {
  auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  std::stringstream res;
  {
    // 1:CryptoContext
    std::stringstream ss;
    Serial::Serialize(cc, ss, SerType::BINARY);
    serialize(SerialTag::CryptoContext, res, ss.str());
  }
  {
    // 2:EvalMultKey(s)
    std::stringstream ss;
    if (!cc->SerializeEvalMultKey(ss, SerType::BINARY)) {
      cerr << "serialize eval mult keys failed" << endl;
    }
    serialize(SerialTag::EvalMultKey, res, ss.str());
  }
  {
    // 3:EvalAutomorphismKey(s) (EvalRotateKey/EvalBootstrapKey)
    std::stringstream ss;
    if (!cc->SerializeEvalAutomorphismKey(ss, SerType::BINARY)) {
      cerr << "serialize eval rotate/bootstrap keys failed" << endl;
    }
    serialize(SerialTag::EvalAutomorphismKey, res, ss.str());
  }
  {
    // 4:PublicKey
    FHEPublicKey fhePubKey(pubKey->getKey());
    serialize(SerialTag::PublicKey, res, fhePubKey.serialize());
  }
  if (contain_sk) {
    // 5:PrivateKey
    FHEPrivateKey fhePriKey(priKey->getKey());
    serialize(SerialTag::PrivateKey, res, fhePriKey.serialize());
  }

  return res.str();
}
void FheKeyset::loads(const std::string &keys) {
  std::stringstream ss(keys);
  size_t beg_pos = 0;
  size_t key_len = keys.length();

  // NOTE: We can't/needn't use get/use cc by
  // auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  // Why? Because openFHE generates a new one when deserializing.
  // So, while all have deserialized, we should set cc.
  // auto cc = CryptoContextMgr::getInstance().getCryptoContext();
  CryptoContext<DCRTPoly> cc;
  cc->ClearEvalMultKeys();
  cc->ClearEvalAutomorphismKeys();
  lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();

  while (beg_pos < key_len) {
    char buffer[4] = {0};

    ss.seekg(beg_pos);
    ss.read(buffer, 4);
    uint32_t _tag = deserializeBE((const uint8_t *)buffer);
    beg_pos += 4;

    ss.seekg(beg_pos);
    ss.read(buffer, 4);
    uint32_t _size = deserializeBE((const uint8_t *)buffer);
    beg_pos += 4;

    std::string ser_data = ss.str().substr(beg_pos, beg_pos + _size);
    beg_pos += _size;
    // cerr << "tag:" << _tag << " size:" << _size << endl;

    std::stringstream _ss(ser_data);
    switch (ToSerialTag(_tag)) {
    case SerialTag::CryptoContext: {
      Serial::Deserialize(cc, _ss, SerType::BINARY);
    } break;
    case SerialTag::EvalMultKey: {
      cc->DeserializeEvalMultKey(_ss, SerType::BINARY);
    } break;
    case SerialTag::EvalAutomorphismKey: {
      cc->DeserializeEvalAutomorphismKey(_ss, SerType::BINARY);
    } break;
    case SerialTag::PublicKey: {
      FHEPublicKey fhePubKey;
      fhePubKey.deserialize(ser_data);
      pubKey = std::make_shared<FHEPublicKey>(fhePubKey.getKey());
    } break;
    case SerialTag::PrivateKey: {
      FHEPrivateKey fhePriKey;
      fhePriKey.deserialize(ser_data);
      priKey = std::make_shared<FHEPrivateKey>(fhePriKey.getKey());
    } break;

    default:
      throw std::invalid_argument("Invalid SerialTag");
      break;
    }
  }

  CryptoContextMgr::getInstance().setCryptoContext(cc);
}

} // namespace openfhe
} // namespace aegiscpu
