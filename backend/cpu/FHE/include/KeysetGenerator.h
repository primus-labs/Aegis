#ifndef CPU_FHE_KEYSETBUILDER_H
#define CPU_FHE_KEYSETBUILDER_H

#include "FheKeyset.h"
#include "Common/Protocol.h"

using mlir::aegis::ProtoMessage;

namespace aegislang {


class KeysetGenerator  {
public:
    static void generate(ProtoMessage<aegisprotocol::KeyInfo>& keyInfo);
};

}

#endif