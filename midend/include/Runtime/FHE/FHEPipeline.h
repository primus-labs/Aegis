#ifndef RUNTIME_FHEPIPELINE_H
#define RUNTIME_FHEPIPELINE_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/IR/Module.h"


namespace mlir {
namespace aegis {
namespace fhepipeline {

mlir::LogicalResult lowerHighLevelMlir(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                       std::function<bool(mlir::Pass *)> enablePass, bool verbose);

mlir::LogicalResult lowerMlirToSecret(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                    std::function<bool(mlir::Pass *)> enablePass, bool verbose);

mlir::LogicalResult lowerSecretToFhe(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                    std::function<bool(mlir::Pass *)> enablePass, bool verbose);

mlir::LogicalResult lowerFheToEmitc(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                    std::function<bool(mlir::Pass *)> enablePass, bool verbose);

mlir::LogicalResult transformEmitcToCpp(mlir::MLIRContext &context, mlir::ModuleOp &module,
                                        std::string &cppFileName, bool verbose);

mlir::LogicalResult emitSharedLibrary(const std::string &cppFileName, std::string &sharedLibName);


} // namespace fhepipeline
} // namespace aegis
} // namespace mlir

#endif // RUNTIME_FHEPIPELINE_H 