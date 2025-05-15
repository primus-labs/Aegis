#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Sequence.h"
#include "Pass/LoadStoreToCopy/LoadStoreToCopy.h"

#define DEBUG_TYPE "loadstore-to-copy"

using namespace mlir;
using namespace aegis;


void LoadStoreToCopyPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, mlir::affine::AffineDialect, func::FuncDialect>();
}


void LoadStoreToCopyPass::runOnOperation() {
    //TODO
}