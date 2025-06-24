#ifndef RUNTIME_SIMULATE_PIPELINE_H
#define RUNTIME_SIMULATE_PIPELINE_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/IR/Module.h"

namespace mlir {
namespace aegis {
namespace simpipeline {

mlir::LogicalResult lowerToLowLevelMLIR(mlir::ModuleOp &moduleOp, const std::string &mlirFullFileName);

} // namespace simpipeline
} // namespace aegis
} // namespace mlir

#endif //RUNTIME_SIMULATE_PIPELINE_H