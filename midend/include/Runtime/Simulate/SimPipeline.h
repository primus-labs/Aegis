#ifndef RUNTIME_SIMULATE_PIPELINE_H
#define RUNTIME_SIMULATE_PIPELINE_H

#include <string_view>
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/IR/Module.h"

namespace mlir {
namespace aegis {
namespace simpipeline {

// clang-format off
constexpr std::string_view kEntryPointFunc = R"cpp(
func.func private @printMemrefF64(memref<*xf64>)
func.func @main() {{
    // deal with argument
    {0} 
    // call func
    {1}
    // memref.cast to real result memref
    {2} 
    call @printMemrefF64({3}) : (memref<*xf64>) -> ()
    func.return
}
)cpp";
// clang-format on

mlir::LogicalResult lowerToSimulateMLIR(mlir::ModuleOp &moduleOp, const std::string &mlirFullFileName);
mlir::LogicalResult addEntryPointFunc(mlir::ModuleOp &moduleOp, const std::string &mlirFullFileName);

} // namespace simpipeline
} // namespace aegis
} // namespace mlir

#endif //RUNTIME_SIMULATE_PIPELINE_H