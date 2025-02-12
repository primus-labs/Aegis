#include "KeysetGenerator.h"

namespace aegislang {

void KeysetGenerator ::generateFheKeyset(FheKeysetInfo ks_info) {
    // Create crypto parameters
    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(ks_info.multDepth);
    parameters.SetScalingModSize(ks_info.scaleModSize);
    parameters.SetBatchSize(ks_info.batchSize);

    // Create crypto context
    CryptoContext<DCRTPoly> cryptoContext = GenCryptoContext(parameters);
    cryptoContext->Enable(PKE);
    cryptoContext->Enable(KEYSWITCH);
    cryptoContext->Enable(LEVELEDSHE);

    // Generate keypairs
    KeyPair<DCRTPoly> keyPair = cryptoContext->KeyGen();

    // Gen Relinearization Key
    cryptoContext->EvalMultKeyGen(keyPair.secretKey);

    // Gen Galois Key
    cryptoContext->EvalRotateKeyGen(keyPair.secretKey, ks_info.galoisIndices);

    // Gen Bootstrapping Key
    if (ks_info.enableBootstrapping) {
        cryptoContext->Enable(FHE);
        cryptoContext->EvalBootstrapKeyGen(keyPair.secretKey, ks_info.numSlot);
    }

    // create Key obj
    auto priKey = std::make_shared<FHEPrivateKey>(keyPair.secretKey);
    auto pubKey = std::make_shared<FHEPublicKey>(keyPair.publicKey);
    auto relinKey = std::make_shared<FHERelinKey>("");
    auto rotKey = std::make_shared<FHERotateKey>("");
    auto bsKey = std::make_shared<FHEBootstrapKey>("");

    // Initialize the singleton FheKeyset
    FheKeyset::initialize(priKey, pubKey, relinKey, rotKey, bsKey);
}

}  //namespace aegislang