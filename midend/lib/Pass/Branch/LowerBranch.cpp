#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "mlir/IR/IRMapping.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/Branch/LowerBranch.h"

#define DEBUG_TYPE "branch"

using namespace mlir;
using namespace aegis;


void BranchPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<mlir::affine::AffineDialect, func::FuncDialect, mlir::scf::SCFDialect>();
}


void BranchPass::runOnOperation() {
    ModuleOp module = getOperation();
    MLIRContext *context = &getContext();

    module.walk([&](scf::IfOp ifOp) {
        // Check number of results (reject multiple results)
        if (ifOp.getNumResults() != 1) {
            ifOp.emitError("Multi-result scf.if is not supported");
            return;
        }

        // Verify block structure
        auto verifyBlock = [&](Block *block) -> LogicalResult {
        if (block->empty() || !isa<scf::YieldOp>(block->back())) {
            return ifOp.emitOpError("Block must end with scf.yield");
        }
            return success();
        };

        if (failed(verifyBlock(ifOp.thenBlock())) || 
            (ifOp.elseBlock() && failed(verifyBlock(ifOp.elseBlock())))) {
            return; 
        }

        // Clone entire then/else blocks
        IRRewriter rewriter(context);
        rewriter.setInsertionPoint(ifOp);
        IRMapping mapper;

        // Clone then block operations
        // Original then block:
        // ^bb(%arg0: type):
        //   %t1 = some_op ...
        //   scf.yield %t1
        Value thenResult;
        {
            Block *thenBlock = ifOp.thenBlock();
            
            // Clone block arguments
            SmallVector<Value> newThenArgs;
            for (auto oldArg : thenBlock->getArguments()) {
                Value newArg = rewriter.getInsertionBlock()->addArgument(
                                                oldArg.getType(), oldArg.getLoc());
                mapper.map(oldArg, newArg);
            }

            // Clone operations except yield
            for (auto &op : llvm::make_range(thenBlock->begin(), std::prev(thenBlock->end()))) {
            rewriter.clone(op, mapper);
            }

            // Get yield result from original block
            thenResult = cast<scf::YieldOp>(thenBlock->back()).getOperand(0);
            thenResult = mapper.lookupOrDefault(thenResult);
        }

        // Clone else block operations (if exists)
        Value elseResult;
        if (ifOp.elseBlock()) {
            Block *elseBlock = ifOp.elseBlock();

            // Clone block arguments
            SmallVector<Value> newElseArgs;
            for (auto oldArg : elseBlock->getArguments()) {
                Value newArg = rewriter.getInsertionBlock()->addArgument(
                                                oldArg.getType(), oldArg.getLoc());
                mapper.map(oldArg, newArg);
            }

            // Clone operations except yield
            for (auto &op : llvm::make_range(elseBlock->begin(), std::prev(elseBlock->end()))) {
                rewriter.clone(op, mapper);
            }

            // Get yield result from original block
            elseResult = mlir::cast<scf::YieldOp>(elseBlock->back()).getOperand(0);
            elseResult = mapper.lookupOrDefault(elseResult);
        }

        // Extract yield results
        auto getYieldResult = [](Block *block) -> Value {
            return cast<scf::YieldOp>(block->back()).getOperand(0);
        };

        // Materialize condition to arithmetic type (e.g. i1 -> f32)
        Type resultType = thenResult.getType();
        Value cond = ifOp.getCondition();

        // Generate arithmetic select logic
        // result = cond * then_val + (1 - cond) * else_val
        Value finalResult;
        if (ifOp.elseBlock()) {
            // // Create 1.0 constant
            // Value one = rewriter.create<arith::ConstantFloatOp>(
            //     ifOp.getLoc(), APFloat(1.0f), cast<FloatType>(resultType));
            
            // // Calculate (1 - cond)
            // Value oneSubCond = rewriter.create<arith::SubFOp>(
            //     ifOp.getLoc(), one, castCond);
            
            // // Multiply terms
            // Value thenTerm = rewriter.create<arith::MulFOp>(
            //     ifOp.getLoc(), castCond, thenResult);

            // Value elseTerm = rewriter.create<arith::MulFOp>(
            //     ifOp.getLoc(), oneSubCond, elseResult);
            
            // // Final addition
            // finalResult = rewriter.create<arith::AddFOp>(
            //     ifOp.getLoc(), thenTerm, elseTerm);
            finalResult = rewriter.create<arith::SelectOp>(
                    ifOp.getLoc(), cond, thenResult, elseResult);
        } else {
            // Using FHE::CastOp converts i1 to float type
            // Value castCond = rewriter.create<arith::UIToFPOp>(
            //     ifOp.getLoc(), resultType, cond);
            Value castCond = rewriter.create<fhe::CastOp>(
                    ifOp.getLoc(), resultType, cond);

            // For no else block: result = cond * then_val
            finalResult = rewriter.create<arith::MulFOp>(
                ifOp.getLoc(), castCond, thenResult);
        }

        // Replace original if operation
        rewriter.replaceOp(ifOp, finalResult);
    });
}
