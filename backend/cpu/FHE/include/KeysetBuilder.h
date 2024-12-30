#ifndef CPU_FHE_KEYSETBUILDER_H
#define CPU_FHE_KEYSETBUILDER_H

#include "FheKeyset.h"


struct FheKeysetInfo {
// poly_modulus_degree
// coff_modulus_chain
// scale
// multDepth
// scaleModSize
// batchSize
};


class KeysetBuilder {
private:

public:
    FheKeyset generateFheKeyset(FheKeysetInfo ks_info);
};

#endif