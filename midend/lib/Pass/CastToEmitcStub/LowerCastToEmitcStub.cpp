#include <memory>
#include <iostream>

#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Conversion/MemRefToEmitC/MemRefToEmitC.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/include/mlir/IR/MLIRContext.h"            
#include "mlir/include/mlir/IR/PatternMatch.h"           
#include "mlir/include/mlir/IR/Value.h"     
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/CastToEmitcStub/LowerCastToEmitcStub.h"

#define DEBUG_TYPE "cast-to-emitc-stub"

using namespace mlir;
using namespace aegis;
using namespace fhe;



// CastPattern replace fhe::CastOp to a dummy emitc function call. 
class CastPattern : public OpRewritePattern<fhe::CastOp> 
{
public:
    using OpRewritePattern<fhe::CastOp>::OpRewritePattern;

    LogicalResult matchAndRewrite(fhe::CastOp op, PatternRewriter &rewriter) const override 
    {
        auto destTy = op.getType();
        auto operand = op.getOperand();
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "Cast_Stub", 
                                    ArrayAttr(), ArrayAttr(), operand);
        return success();
    }
};


void LowerCastToEmitcStubPass::getDependentDialects(mlir::DialectRegistry &registry) const
{
    registry.insert<fhe::FHEDialect>();
}


void LowerCastToEmitcStubPass::runOnOperation() 
{
    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<CastPattern>(&getContext());
    (void)applyPatternsAndFoldGreedily(getOperation(), std::move(patterns));
}