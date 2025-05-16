#include "Pass/FoldArithChain/FoldArithChain.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"
#include <iostream>
#include <memory>

using namespace mlir;
using namespace aegis;

void FoldArithChainPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, mlir::affine::AffineDialect, func::FuncDialect,
                    mlir::scf::SCFDialect, mlir::tensor::TensorDialect>();
}

// Add fold
void collapseAdd(fhe::LWEAddOp &op, IRRewriter &rewriter) {
    for (auto &use : llvm::make_early_inc_range(op.getOutput().getUses())) {
        if (auto use_add = llvm::dyn_cast<fhe::LWEAddOp>(*use.getOwner())) {
            rewriter.setInsertionPointAfter(use_add.getOperation());
            llvm::SmallVector<Value, 4> new_operands;
            for (auto s : use_add.getX()) {
                if (s != op.getOutput()) {
                    new_operands.push_back(s);
                }
            }
            for (auto s : op.getX()) {
                new_operands.push_back(s);
            }
            auto new_add =
                rewriter.create<fhe::LWEAddOp>(use_add->getLoc(), use_add.getOutput().getType(), new_operands);
            use_add.replaceAllUsesWith(new_add.getOperation());
        }
    }
}

// Sub fold
void collapseSub(fhe::LWESubOp &op, IRRewriter &rewriter) {
    for (auto &use : llvm::make_early_inc_range(op.getOutput().getUses())) {
        if (auto use_sub = llvm::dyn_cast<fhe::LWESubOp>(*use.getOwner())) {
            rewriter.setInsertionPointAfter(use_sub.getOperation());
            llvm::SmallVector<Value, 4> new_operands;
            for (auto s : use_sub.getX()) {
                if (s != op.getOutput()) {
                    new_operands.push_back(s);
                }
            }
            for (auto s : op.getX()) {
                new_operands.push_back(s);
            }
            auto new_sub =
                rewriter.create<fhe::LWESubOp>(use_sub->getLoc(), use_sub.getOutput().getType(), new_operands);
            use_sub.replaceAllUsesWith(new_sub.getOperation());
        }
    }
}

// Mul fold
void collapseMul(fhe::LWEMulOp &op, IRRewriter &rewriter) {
    for (auto &use : llvm::make_early_inc_range(op.getOutput().getUses())) {
        if (auto use_mul = llvm::dyn_cast<fhe::LWEMulOp>(*use.getOwner())) {
            rewriter.setInsertionPointAfter(use_mul.getOperation());
            llvm::SmallVector<Value, 4> new_operands;
            for (auto s : use_mul.getX()) {
                if (s != op.getOutput()) {
                    new_operands.push_back(s);
                }
            }
            for (auto s : op.getX()) {
                new_operands.push_back(s);
            }
            auto new_mul =
                rewriter.create<fhe::LWEMulOp>(use_mul->getLoc(), use_mul.getOutput().getType(), new_operands);
            use_mul.replaceAllUsesWith(new_mul.getOperation());
        }
    }
}

// add/sub/mult operation aggragation to get ready for batching optimizatins
void FoldArithChainPass::runOnOperation() {
    IRRewriter rewriter(&getContext());
    auto module = getOperation();
    module.walk([&](func::FuncOp funcOp) {
        llvm::SmallVector<Operation *, 16> ops;
        funcOp.walk([&](Operation *op) { ops.push_back(op); });
        for (auto *op : llvm::reverse(ops)) {
            if (op->getUsers().empty() && (isa<fhe::CastOp>(op) || isa<arith::ConstantOp>(op))) {
                rewriter.eraseOp(op);
            }
        }
    });
    ConversionTarget target(getContext());
    target.addLegalDialect<affine::AffineDialect, func::FuncDialect, tensor::TensorDialect, scf::SCFDialect>();
    target.addIllegalOp<affine::AffineForOp>();

    // Get the default block in the module's only region
    auto &block = getOperation()->getRegion(0).getBlocks().front();

    for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        for (auto op : llvm::make_early_inc_range(f.getBody().getOps<arith::SubIOp>())) {
            rewriter.setInsertionPointAfter(op.getOperation());
            llvm::SmallVector<Value, 2> operands = {op.getLhs(), op.getRhs()};
            Value value = rewriter.create<fhe::LWESubOp>(op.getLoc(), op.getResult().getType(), operands);
            op.replaceAllUsesWith(value);
            rewriter.eraseOp(op.getOperation());
        }
    }

    for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        for (auto op : llvm::make_early_inc_range(f.getBody().getOps<arith::MulIOp>())) {
            rewriter.setInsertionPointAfter(op.getOperation());
            llvm::SmallVector<Value, 2> operands = {op.getLhs(), op.getRhs()};
            Value value = rewriter.create<fhe::LWEMulOp>(op.getLoc(), op.getResult().getType(), operands);
            op.replaceAllUsesWith(value);
            rewriter.eraseOp(op.getOperation());
        }
    }

    // Now, go through and actually start collapsing the operations!
    for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        for (auto op : llvm::make_early_inc_range(f.getBody().getOps<fhe::LWEAddOp>())) {
            collapseAdd(op, rewriter);
        }
    }

    for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        for (auto op : llvm::make_early_inc_range(f.getBody().getOps<fhe::LWESubOp>())) {
            collapseSub(op, rewriter);
        }
    }

    // for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
    //     for (auto op : llvm::make_early_inc_range(f.getBody().getOps<fhe::LWEMulOp>())) {
    //         collapseMul(op, rewriter);
    //     }
    // }
}
