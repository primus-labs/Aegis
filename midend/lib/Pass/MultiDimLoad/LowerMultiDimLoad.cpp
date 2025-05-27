#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Utils/IndexingUtils.h"
#include "mlir/Dialect/Utils/StaticValueUtils.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Sequence.h"
#include "Pass/MultiDimLoad/LowerMultiDimLoad.h"

#define DEBUG_TYPE "multi-dim-load"

using namespace mlir;
using namespace aegis;

void LowerMultiDimLoadPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, func::FuncDialect>();
}

void LowerMultiDimLoadPass::runOnOperation() {
    //TODO
}