#include <iostream>
#include <memory>

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/Utils.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/IR/PatternMatch.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "Pass/ArithToSecret/LowerArithToSecret.h"

#define DEBUG_TYPE "arith-to-secret"

using namespace mlir;
using namespace aegis;
using namespace secret;


// Helper function to check if a value is encrypted
bool isEncrypted(Value value, llvm::DenseMap<Value, bool> &cache) {
    // Check if the value is already in the cache
    auto it = cache.find(value);
    if (it != cache.end()) {
        return it->second;
    }

    // If the value is a BlockArgument, check its attribute
    if (auto arg = mlir::dyn_cast<BlockArgument>(value)) {
        if (auto funcOp = mlir::dyn_cast<func::FuncOp>(arg.getOwner()->getParentOp())) {
            if (auto attr = funcOp.getArgAttr(arg.getArgNumber(), "type")) {
                bool result = (mlir::cast<StringAttr>(attr).getValue() == "encrypted");
                cache[value] = result;
                return result;
            }
        }

        cache[value] = false;
        return false;
    }

    // If the value is an operation result, check its defining operation
    if (auto opResult = mlir::dyn_cast<OpResult>(value)) {
        Operation *op = opResult.getDefiningOp();

        // If the operation is a secret operation, the result is encrypted
        if (isa<SecretDialect>(op->getDialect())) {
            cache[value] = true;
            return true;
        }

        // For any operation, check if any of its operands is encrypted
        for (Value operand : op->getOperands()) {
            if (isEncrypted(operand, cache)) {
                cache[value] = true;
                return true;
            }
        }

        // Default to false if no conditions are met
        cache[value] = false;
        return false;
    }

    // Default to false if the value is neither a BlockArgument nor an OpResult
    cache[value] = false;
    return false;
}

// Pattern to convert arith.add to secret.add or secret.add_plain
struct ConvertAddOpPat : public OpRewritePattern<arith::AddFOp> {
    using OpRewritePattern<arith::AddFOp>::OpRewritePattern;

    LogicalResult matchAndRewrite(arith::AddFOp op, PatternRewriter &rewriter) const override {
        Value lhs = op.getLhs();
        Value rhs = op.getRhs();

        llvm::DenseMap<Value, bool> cache;
        bool lhsEncrypted = isEncrypted(lhs, cache);
        bool rhsEncrypted = isEncrypted(rhs, cache);

        if (!lhsEncrypted && !rhsEncrypted) {
            return failure();
        }

        if (lhsEncrypted && !mlir::isa<SecretType>(lhs.getType())) {
            lhs = rewriter.create<ConcealOp>(op.getLoc(), SecretType::get(op.getContext(), lhs.getType()), lhs);
        }
        if (rhsEncrypted && !mlir::isa<SecretType>(rhs.getType())) {
            rhs = rewriter.create<ConcealOp>(op.getLoc(), SecretType::get(op.getContext(), rhs.getType()), rhs);
        }

        if (lhsEncrypted && rhsEncrypted) {
            rewriter.replaceOpWithNewOp<AddOp>(op, SecretType::get(op.getContext(), op.getType()), lhs, rhs);
        } else {
            rewriter.replaceOpWithNewOp<AddPlainOp>(op, SecretType::get(op.getContext(), op.getType()), lhs, rhs);
        }

        return success();
    }
};


// Pattern to convert arith.sub to secret.sub
struct ConvertSubOpPat : public OpRewritePattern<arith::SubFOp> {
    using OpRewritePattern<arith::SubFOp>::OpRewritePattern;

    LogicalResult matchAndRewrite(arith::SubFOp op, PatternRewriter &rewriter) const override {
        Value lhs = op.getLhs();
        Value rhs = op.getRhs();

        llvm::DenseMap<Value, bool> cache;
        bool lhsEncrypted = isEncrypted(lhs, cache);
        bool rhsEncrypted = isEncrypted(rhs, cache);

        if (!lhsEncrypted && !rhsEncrypted) {
            return failure();
        }

        if (lhsEncrypted && !mlir::isa<SecretType>(lhs.getType())) {
            lhs = rewriter.create<ConcealOp>(op.getLoc(), SecretType::get(op.getContext(), lhs.getType()), lhs);
        }
        if (rhsEncrypted && !mlir::isa<SecretType>(rhs.getType())) {
            rhs = rewriter.create<ConcealOp>(op.getLoc(), SecretType::get(op.getContext(), rhs.getType()), rhs);
        }

        if (lhsEncrypted && rhsEncrypted) {
            rewriter.replaceOpWithNewOp<SubOp>(op, SecretType::get(op.getContext(), op.getType()), lhs, rhs);
        } else {
            rewriter.replaceOpWithNewOp<SubPlainOp>(op, SecretType::get(op.getContext(), op.getType()), lhs, rhs);
        }

        return success();
    }
};


// Pattern to convert arith.mul to secret.mul or secret.mul_plain
struct ConvertMulOpPat : public OpRewritePattern<arith::MulFOp> {
    using OpRewritePattern<arith::MulFOp>::OpRewritePattern;

    LogicalResult matchAndRewrite(arith::MulFOp op, PatternRewriter &rewriter) const override {
        Value lhs = op.getLhs();
        Value rhs = op.getRhs();

        llvm::DenseMap<Value, bool> cache;
        bool lhsEncrypted = isEncrypted(lhs, cache);
        bool rhsEncrypted = isEncrypted(rhs, cache);

        // If both operands are clear, no conversion is needed
        if (!lhsEncrypted && !rhsEncrypted) {
            return failure();
        }

        // Convert operands to Secret type if necessary
        if (lhsEncrypted && !mlir::isa<SecretType>(lhs.getType())) {
            lhs = rewriter.create<ConcealOp>(op.getLoc(), SecretType::get(op.getContext(), lhs.getType()), lhs);
        }
        if (rhsEncrypted && !mlir::isa<SecretType>(rhs.getType())) {
            rhs = rewriter.create<ConcealOp>(op.getLoc(), SecretType::get(op.getContext(), rhs.getType()), rhs);
        }

        // Create the appropriate secret operation
        if (lhsEncrypted && rhsEncrypted) {
            rewriter.replaceOpWithNewOp<MulOp>(op, SecretType::get(op.getContext(), op.getType()), lhs, rhs);
        } else {
            rewriter.replaceOpWithNewOp<MulPlainOp>(op, SecretType::get(op.getContext(), op.getType()), lhs, rhs);
        }

        return success();
    }
};




void LowerArithToSecretPass::getDependentDialects(mlir::DialectRegistry &registry) const 
{
    registry.insert<arith::ArithDialect>();
    registry.insert<affine::AffineDialect>();
    registry.insert<func::FuncDialect>();
    registry.insert<secret::SecretDialect>();
    registry.insert<scf::SCFDialect>();
    registry.insert<memref::MemRefDialect>();
}

void LowerArithToSecretPass::runOnOperation() {
    MLIRContext *context = &getContext();
    RewritePatternSet patterns(context);

    // Add patterns for converting arithmetic operations to secret operations
    patterns.add<ConvertAddOpPat, ConvertSubOpPat, ConvertMulOpPat>(context);

    if (failed(applyPatternsAndFoldGreedily(getOperation(), std::move(patterns)))) {
        signalPassFailure();
    }
}
