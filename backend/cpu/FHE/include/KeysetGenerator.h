#ifndef CPU_FHE_KEYSETBUILDER_H
#define CPU_FHE_KEYSETBUILDER_H

#include "Common/Protocol.h"
#include "FheKeyset.h"

using mlir::aegis::ProtoMessage;

namespace aegiscpu {
namespace openfhe {

class KeysetGenerator {
  public:
    static void generate(ProtoMessage<aegisprotocol::KeyInfo> &keyInfo);
};

} // namespace openfhe {
} // namespace aegiscpu

#endif