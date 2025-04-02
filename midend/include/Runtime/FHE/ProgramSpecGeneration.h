#ifndef RUNTIME_FHE_PROGRAMSPEC_GENERATION_H
#define RUNTIME_FHE_PROGRAMSPEC_GENERATION_H

#include <memory>
#include "mlir/IR/BuiltinOps.h"
#include "Common/Protocol.h"

using mlir::aegis::ProtoMessage;

namespace mlir {
namespace aegis {

llvm::Expected<ProtoMessage<aegisprotocol::ProgSpec>> 
    createProgramSpec(mlir::ModuleOp module);

} // namespace aegis
} // namespace mlir

#endif // RUNTIME_FHE_PROGRAMSPEC_GENERATION_H