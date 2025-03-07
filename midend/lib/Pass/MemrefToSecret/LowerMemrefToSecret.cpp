#include <iostream>
#include <memory>

#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/IR/Builders.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "Pass/MemrefToSecret/LowerMemrefToSecret.h"
#include "Common/MetadataMgr.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "memref-to-secret"

using namespace mlir;
using namespace aegis;
using namespace secret;


void LowerMemrefToSecretPass::getDependentDialects(mlir::DialectRegistry &registry) const  {
    registry.insert<func::FuncDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<arith::ArithDialect>();
    registry.insert<secret::SecretDialect>();
}


void runOnOperation() {
    //TODO
}


