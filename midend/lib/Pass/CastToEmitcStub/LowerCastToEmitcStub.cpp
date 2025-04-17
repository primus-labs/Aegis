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
        if (auto constantOp = mlir::dyn_cast_or_null<emitc::ConstantOp>(operand.getDefiningOp())) {
            // Get value attribute
            Attribute valueAttr = constantOp.getValueAttr();

            // Exist OpaqueAttr ?
            if (auto opaqueAttr = mlir::dyn_cast<emitc::OpaqueAttr>(valueAttr)) {
                StringRef valueStr = opaqueAttr.getValue();
                if (valueStr.starts_with("MakePlain") && mlir::isa<mlir::IndexType>(destTy)) {
                    rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "Cast_Plain_To_Index", 
                                    ArrayAttr(), ArrayAttr(), operand);
                    return success();
                }
            }
        } else {
            llvm::errs() << "Unexpected execution path reached in LowerCastToEmitcStubPass::matchAndRewrite, "
                         << "Possible incompatible casting operation found.\n"; 
        }

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