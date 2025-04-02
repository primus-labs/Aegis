#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <variant>

#include "capnp/message.h"
#include "protocol.capnp.h"
#include "Runtime/FHE/ProgramSpecGeneration.h"
#include "Common/Protocol.h"
#include "Common/Error.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/abi-breaking.h"
#include "llvm/Support/Error.h"



llvm::Expected<ProtoMessage<aegisprotocol::ProgSpec>> createProgramSpec(mlir::ModuleOp module) {
    return ErrorMsg("Not yet implement");
}
