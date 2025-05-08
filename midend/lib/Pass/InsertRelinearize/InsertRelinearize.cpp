#include "Pass/InsertRelinearize/InsertRelinearize.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Transforms/DialectConversion.h"
#include "llvm/ADT/APSInt.h"

#define DEBUG_TYPE "insert-relinearize"

using namespace mlir;
using namespace aegis;


void InsertRelinearizePass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect>();
}

// In OpenFHE, relinearization is automatically performed by default after every multiplication operation(EvalMult), 
// eg:Ciphertext c3 = EvalMult(c1, c2), the Relinearize is auto-applied,
// eliminating the need for manual insertion of redundant relinearization steps.
void InsertRelinearizePass::runOnOperation() {
    //TODO
}