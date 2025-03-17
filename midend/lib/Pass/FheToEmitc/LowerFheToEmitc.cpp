#include <string>

#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Transforms/DialectConversion.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/FheToEmitc/LowerFheToEmitc.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "fhe-to-emitc"

using namespace mlir;
using namespace aegis;
using namespace fhe;


void LowerFheToEmitcPass::getDependentDialects(mlir::DialectRegistry &registry) const
{
    registry.insert<func::FuncDialect>();
    registry.insert<mlir::emitc::EmitCDialect>();
}


void LowerFheToEmitcPass::runOnOperation()
{
    //TODO
}