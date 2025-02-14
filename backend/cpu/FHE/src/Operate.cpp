#include "Operate.h"
#include "CryptoContextMgr.h"
#include "FheKeyset.h"
//#include "Common/ProgramSpec.h"

namespace aegiscpu {

std::vector<uint8_t> encrypt(std::vector<T>& data) {
    CryptoContext<DCRTPoly> cc = CryptoContextMgr::getInstance().getCryptoContext();

    //ProtoMessage<aegisprotocol::KeyInfo> keyInfo = ProgramSpec::getInstance().getKeyInfo();
    Plaintext ptValue = cc->MakeCKKSPackedPlaintext(data);

    PublicKey<DCRTPoly> pk = FheKeyset::getInstance.getPubKey()->getKey();
    Ciphertext<DCRTPoly> ctValue = cc->Encrypt(pk, ptValue);
    std::stringstream ss;
    Serial::Serialize(ctValue, ss, SerType::BINARY);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ss)),
                                       std::istreambuf_iterator<char>());
    
    return buffer;
}

} //namespace aegiscpu