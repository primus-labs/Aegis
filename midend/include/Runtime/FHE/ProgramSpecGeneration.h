#ifndef RUNTIME_FHE_PROGRAMSPEC_GENERATION_H
#define RUNTIME_FHE_PROGRAMSPEC_GENERATION_H

#include <memory>
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "Common/Protocol.h"
#include "Runtime/CompilerEngine.h"

using mlir::aegis::ProtoMessage;

namespace mlir {
namespace aegis {

llvm::Expected<ProtoMessage<aegisprotocol::ProgSpec>> createProgramSpec(mlir::ModuleOp module, CompileOptions options);

llvm::Expected<ProtoMessage<aegisprotocol::Functions>> getAllFunctionsInfo(mlir::ModuleOp module);

llvm::Expected<ProtoMessage<aegisprotocol::Function>> getUnitFunctionInfo(mlir::func::FuncOp funcOp);

llvm::Expected<ProtoMessage<aegisprotocol::FuncParam>> getFuncParamFromType(mlir::Type ty, const std::vector<int> dims);

llvm::Expected<ProtoMessage<aegisprotocol::KeyInfo>> getKeyInfo(mlir::ModuleOp module, CompileOptions options);

llvm::Expected<ProtoMessage<aegisprotocol::StatsInfo>> getStatsInfo(mlir::ModuleOp module);

} // namespace aegis
} // namespace mlir

#endif // RUNTIME_FHE_PROGRAMSPEC_GENERATION_H