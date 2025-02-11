#ifndef CPU_FHE_KEYSETBUILDER_H
#define CPU_FHE_KEYSETBUILDER_H

#include "FheKeyset.h"

namespace aegislang {

struct FheKeysetInfo {
    size_t polyModulusDegree;               // Polynomial modulus degree
    std::vector<size_t> coefModulusChain;   // Coefficient modulus chain
    size_t scale;                           // Scale factor
    size_t multDepth;                       // Multiplication depth
    size_t scaleModSize;                    // Scale modulus size
    size_t batchSize;                       // Batch size
    std::vector<int> galoisIndices;         // Index list for Galois Key
    bool enableBootstrapping;               // Whether to enable bootstrapping
    size_t numSlot;                         // number of slots
};


class KeysetBuilder {
public:
    static std::shared_ptr<FheKeyset> generateFheKeyset(FheKeysetInfo ks_info);
};

}

#endif