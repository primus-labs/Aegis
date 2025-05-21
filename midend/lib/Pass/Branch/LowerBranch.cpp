#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "Pass/Branch/LowerBranch.h"

#define DEBUG_TYPE "branch"

using namespace mlir;


void BranchPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<mlir::affine::AffineDialect, func::FuncDialect, mlir::scf::SCFDialect>();
}


void BranchPass::runOnOperation() {
    //TODO
}