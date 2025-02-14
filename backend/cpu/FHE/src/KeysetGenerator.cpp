#include "KeysetGenerator.h"
#include "CryptoContextMgr.h"
#include "Common/Protocol.h"


namespace aegislang {

void KeysetGenerator ::generate(ProtoMessage<aegisprotocol::KeyInfo>& keyInfo) {
    static std::once_flag initFlag;

    std::call_once(initFlag, [&]() {
        // Create crypto parameters
        CCParams<CryptoContextCKKSRNS> parameters;
        parameters.SetMultiplicativeDepth(keyInfo.asReader().getMultDepth());
        parameters.SetScalingModSize(keyInfo.asReader().getScaleModSize());
        parameters.SetBatchSize(keyInfo.asReader().getBatchSize());

        // Create crypto context
        CryptoContext<DCRTPoly> cryptoContext = CryptoContextMgr::getInstance().getCryptoContext(parameters);
        cryptoContext->Enable(PKE);
        cryptoContext->Enable(KEYSWITCH);
        cryptoContext->Enable(LEVELEDSHE);

        // Generate keypairs
        KeyPair<DCRTPoly> keyPair = cryptoContext->KeyGen();

        // Gen Relinearization Key
        cryptoContext->EvalMultKeyGen(keyPair.secretKey);

        // Gen Galois Key
        std::vector<int> galoisIdx;
        for (auto ind : keyInfo.asReader().getGaloisIndices()) {
            galoisIdx.push_back(ind);
        }
        cryptoContext->EvalRotateKeyGen(keyPair.secretKey, galoisIdx);

        // Gen Bootstrapping Key
        if (keyInfo.asReader().getEnableBootstrapping()) {
            cryptoContext->Enable(FHE);
            cryptoContext->EvalBootstrapKeyGen(keyPair.secretKey, keyInfo.asReader().getNumSlot());
        }

        // create Key obj
        auto priKey = std::make_shared<FHEPrivateKey>(keyPair.secretKey);
        auto pubKey = std::make_shared<FHEPublicKey>(keyPair.publicKey);
        auto relinKey = std::make_shared<FHERelinKey>("");
        auto rotKey = std::make_shared<FHERotateKey>("");
        auto bsKey = std::make_shared<FHEBootstrapKey>("");

        // Initialize the singleton FheKeyset
        FheKeyset::initialize(priKey, pubKey, relinKey, rotKey, bsKey);
    });
}

}  //namespace aegislang