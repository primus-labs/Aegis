#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/AutoBootstrap/AutoBootstrap.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "auto-bootstrap"

using namespace mlir;
using namespace aegis;

void AutoBootstrapPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect>();
}

void AutoBootstrapPass::runOnOperation() {
    ModuleOp module = getOperation();

    // Phase 1: Analyze multiplication chains across entire module
    module.walk([&](Operation *op) {
        if (isMulOp(op)) {
            processMulOp(op);
        }
    });

    // Phase 2: Insert bootstrap operations at recorded points
    for (auto &point : insertionPoints) {
        insertBootstrapOp(point.first, point.second);
    }
}

void AutoBootstrapPass::processMulOp(Operation *mulOp) {
    Value result = mulOp->getResult(0);

    // Calculate current depth as max of operands' depths + 1
    unsigned maxDepth = 1;
    for (Value o : mulOp->getOperands()) {
        maxDepth = std::max(maxDepth, getChainDepth(o));
    }
    unsigned currentDepth = maxDepth + 1;
    valueChainDepth[result] = currentDepth;
    LLVM_DEBUG(llvm::dbgs() << "current mul op:" << *mulOp << ", depth:" << currentDepth << "\n");
    

    // Trigger bootstrap after 8 consecutive multiplications
    if (currentDepth >= FHE_MAX_MUL_DEPTH) {
        enableFheBoostrapFlag = true;
        recordInsertionPoint(mulOp, result);
        resetChainDepth(result);
    }
}

unsigned AutoBootstrapPass::getChainDepth(Value val) {
    auto it = valueChainDepth.find(val);
    if (it != valueChainDepth.end()) {
        return it->second;
    }

    // Reset depth for non-multiplication inputs
    return (val.getDefiningOp() && !isMulOp(val.getDefiningOp())) ? 0 : 1;
}

void AutoBootstrapPass::insertBootstrapOp(Operation *insertAfter, Value val) {
    OpBuilder builder(insertAfter->getContext());
    builder.setInsertionPointAfter(insertAfter);

    // Create bootstrap operation
    auto bootOp = builder.create<fhe::BootstrapOp>(insertAfter->getLoc(), val.getType(), val);
    Value bootResult = bootOp->getResult(0);

    // Conditional replacement using dominance check
    val.replaceUsesWithIf(bootResult, [&](OpOperand &use) {
        Operation *user = use.getOwner();
        
        // Only replace uses that appear AFTER the bootstrap op
        // in the same block and preserve original dominance
        if (user->getBlock() != bootOp->getBlock()) {
            return false;
        }
        return bootOp->isBeforeInBlock(user);
    });
}

bool AutoBootstrapPass::isMulOp(Operation *op) {
    if (mlir::isa<fhe::LWEMulOp>(op)  || mlir::isa<fhe::LWEMulPlainOp>(op) ||
        mlir::isa<fhe::RLWEMulOp>(op) || mlir::isa<fhe::RLWEMulPlainOp>(op)) {
        return true;
    } else {
        return false;
    }
}