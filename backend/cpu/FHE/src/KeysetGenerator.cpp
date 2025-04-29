#include "KeysetGenerator.h"
#include "Common/Protocol.h"
#include "CryptoContextMgr.h"

namespace aegiscpu {

void KeysetGenerator::generate(ProtoMessage<aegisprotocol::KeyInfo> &keyInfo) {
    static std::once_flag initFlag;

    std::call_once(initFlag, [&]() {
        // Get Multiplicative Depth value
        std::vector<uint32_t> levelBudget = {3, 3};
        unsigned mulDepth = keyInfo.asReader().getMultDepth();
        if (keyInfo.asReader().getEnableBootstrapping()) {
            SecretKeyDist secretKeyDist = UNIFORM_TERNARY;
            mulDepth += FHECKKSRNS::GetBootstrapDepth(levelBudget, secretKeyDist);
        }

        // Create crypto parameters
        CCParams<CryptoContextCKKSRNS> parameters;
        parameters.SetMultiplicativeDepth(mulDepth);
        parameters.SetScalingModSize(keyInfo.asReader().getScaleModSize());
        parameters.SetFirstModSize(keyInfo.asReader().getFirstModSize());
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

        // Gen Rotate Key
        std::vector<int> galoisIdx;
        for (auto ind : keyInfo.asReader().getGaloisIndices()) {
            galoisIdx.push_back(ind);
        }
        if (galoisIdx.size() > 0) {
            cryptoContext->EvalRotateKeyGen(keyPair.secretKey, galoisIdx);
        }

        // Gen Bootstrapping Key
        if (keyInfo.asReader().getEnableBootstrapping()) {
            cryptoContext->Enable(ADVANCEDSHE);
            cryptoContext->Enable(FHE);

            // Precomputations for bootstrapping
            std::vector<uint32_t> bsgsDim = {0, 0};
            cryptoContext->EvalBootstrapSetup(levelBudget, bsgsDim, keyInfo.asReader().getBatchSize());

            // Generate bootstrapping keys.
            cryptoContext->EvalBootstrapKeyGen(keyPair.secretKey, keyInfo.asReader().getBatchSize());
        }

        // create Key obj
        auto priKey = std::make_shared<FHEPrivateKey>(keyPair.secretKey);
        auto pubKey = std::make_shared<FHEPublicKey>(keyPair.publicKey);

        // Initialize the singleton FheKeyset
        FheKeyset::initialize(priKey, pubKey);
    });
}

} // namespace aegiscpu