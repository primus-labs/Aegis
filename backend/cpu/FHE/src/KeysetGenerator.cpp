#include "KeysetGenerator.h"
#include "Common/Protocol.h"
#include "CryptoContextMgr.h"

namespace aegiscpu {
namespace openfhe {

void KeysetGenerator::generate(ProtoMessage<aegisprotocol::KeyInfo> &keyInfo) {
    static std::once_flag initFlag;

    std::call_once(initFlag, [&]() {
        // Get Multiplicative Depth value
        std::vector<uint32_t> levelBudget = {3, 1};
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
        parameters.SetScalingTechnique(FLEXIBLEAUTO);
        parameters.SetBatchSize(keyInfo.asReader().getBatchSize());

        // Create crypto context
        CryptoContext<DCRTPoly> cryptoContext = CryptoContextMgr::getInstance().getCryptoContext(parameters);
        cryptoContext->Enable(PKE);
        cryptoContext->Enable(KEYSWITCH);
        cryptoContext->Enable(LEVELEDSHE);
        cryptoContext->Enable(ADVANCEDSHE);

        // Precomputations for bootstrapping
         if (keyInfo.asReader().getEnableBootstrapping()) {
            cryptoContext->Enable(FHE);

            // Precomputations for bootstrapping
            std::vector<uint32_t> bsgsDim = {0, 0};
            cryptoContext->EvalBootstrapSetup(levelBudget, bsgsDim, keyInfo.asReader().getBatchSize());
         }

        // Generate keypairs
        KeyPair<DCRTPoly> keyPair = cryptoContext->KeyGen();

        // Generate Relinearization Key
        cryptoContext->EvalMultKeyGen(keyPair.secretKey);

        // Generate Rotate Key
        std::vector<int> galoisIdx;
        for (auto ind : keyInfo.asReader().getGaloisIndices()) {
            galoisIdx.push_back(ind);
        }
        if (galoisIdx.size() > 0) {
            cryptoContext->EvalRotateKeyGen(keyPair.secretKey, galoisIdx);
        }

        // Generate Bootstrapping Key
        if (keyInfo.asReader().getEnableBootstrapping()) {
            cryptoContext->EvalBootstrapKeyGen(keyPair.secretKey, keyInfo.asReader().getBatchSize());
        }

        // create Key obj
        auto priKey = std::make_shared<FHEPrivateKey>(keyPair.secretKey);
        auto pubKey = std::make_shared<FHEPublicKey>(keyPair.publicKey);

        // Initialize the singleton FheKeyset
        FheKeyset::initialize(priKey, pubKey);
    });
}

} // namespace openfhe
} // namespace aegiscpu