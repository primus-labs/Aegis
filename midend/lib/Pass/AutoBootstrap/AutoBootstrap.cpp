#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/AutoBootstrap/AutoBootstrap.h"

#define DEBUG_TYPE "auto-bootstrap"

using namespace mlir;
using namespace aegis;


void AutoBootstrapPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect>();
}

void AutoBootstrapPass::runOnOperation() {
    //TODO
}