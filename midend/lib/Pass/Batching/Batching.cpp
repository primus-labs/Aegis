#include <queue>

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Utils/IndexingUtils.h"
#include "mlir/Dialect/Utils/StaticValueUtils.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/Batching/Batching.h"

#define DEBUG_TYPE "batching"

using namespace mlir;
using namespace aegis;

// In OpenFHE and other FHE libraries, the implementation of ​​homomorphic rotation​​ differs, 
// particularly in terms of ​​rotation direction​​ and ​​parameter definitions​​, which require special attention. 
// In OpenFHE, ​​positive numbers​​ represent a ​​left cyclic shift​​, while ​​negative numbers​​ correspond to a ​​right cyclic shift​​.
static constexpr bool NegativeShiftRight = true;    //default for OpenFHE

template <typename OpType>
LogicalResult batchArithOperation(IRRewriter &rewriter, MLIRContext *context, OpType op) {
    // We care only about ops that return scalars, assuming others are already "SIMD-compatible"
    if (mlir::isa<fhe::LWECipherType>(op.getType())) {
        int target_row  = -1; //(-1 => cipher vector not cipher matrix)
        int target_slot = -1; //(-1 => no target slot required)

        // We only search one level deep for now.
        for (auto u : op->getUsers()) {
            if (fhe::LoadOp loadOp = mlir::dyn_cast_or_null<fhe::LoadOp>(u)) {
                assert(static_cast<int64_t>(loadOp.getIndices().size()) <= 2 && "LoadOp indices size > 2 not support");
                if (loadOp.getIndices().size() == 1) {
                    auto idx = getConstantIntValue(loadOp.getIndices()[0]);
                    assert(idx.has_value());
                    target_slot = idx.value();
                } else if (loadOp.getIndices().size() == 2) {
                    auto row = getConstantIntValue(loadOp.getIndices()[0]);
                    auto idx = getConstantIntValue(loadOp.getIndices()[1]);
                    assert(row.has_value() && idx.has_value());
                    target_row = row.value();
                    target_slot = idx.value();
                }
                break;
            } else if (fhe::StoreOp storeOp = mlir::dyn_cast_or_null<fhe::StoreOp>(u)) {
                assert(static_cast<int64_t>(storeOp.getIndices().size()) <= 2 && "StoreOp indices size > 2 not support");
                if (storeOp.getIndices().size() == 1) {
                    auto idx = getConstantIntValue(storeOp.getIndices()[0]);
                    assert(idx.has_value());
                    target_slot = idx.value();
                } else if (storeOp.getIndices().size() == 2) {
                    auto row = getConstantIntValue(storeOp.getIndices()[0]);
                    auto idx = getConstantIntValue(storeOp.getIndices()[1]);
                    assert(row.has_value() && idx.has_value());
                    target_row = row.value();
                    target_slot = idx.value();
                }
                break;
            } else if (auto retOp = mlir::dyn_cast_or_null<func::ReturnOp>(u)) {
                if (mlir::isa<fhe::LWECipherType>(retOp->getOperandTypes().front())) {
                    // we eventually want this as a scalar, which means slot 0
                    target_row = 0;
                    target_slot = 0;
                }
                break;
            }
        }

        // instead of just picking the first target slot we see, we check if we can find 0
        if (target_slot == -1) {
            for (auto it = op->operand_begin(); it != op->operand_end(); ++it) {
                if (mlir::isa<fhe::LWECipherType>((*it).getType())) {
                    // scalar-type input that needs to be converted
                    if (fhe::LoadOp loadOp = (*it).template getDefiningOp<fhe::LoadOp>()) {
                        assert(static_cast<int64_t>(loadOp.getIndices().size()) <= 2 && "LoadOp indices size > 2 not support");
                        if (loadOp.getIndices().size() == 1) {
                            auto idx = getConstantIntValue(loadOp.getIndices()[0]);
                            assert(idx.has_value());
                            auto i = idx.value();
                            if (target_slot == -1) {
                                target_slot = i;
                                break;
                            }
                        } else if (loadOp.getIndices().size() == 2) {
                            auto row = getConstantIntValue(loadOp.getIndices()[0]);
                            auto idx = getConstantIntValue(loadOp.getIndices()[1]);
                            assert(row.has_value() && idx.has_value());
                            auto r = row.value();
                            auto i = idx.value();
                            if (target_row == -1) {
                                target_row = r;
                            }
                            if (target_slot == -1) {
                                target_slot = i;
                            }
                            break;
                        } 
                    }
                }
            }
        }

        // create new op, and to avoid any operand transformation operations being after the new operation.
        rewriter.setInsertionPointAfter(op);
        auto new_op = rewriter.create<OpType>(op.getLoc(), op.getType(), op->getOperands());
        rewriter.setInsertionPoint(new_op);

        // find the maximum size of vector involved
        int max_size = -1;
        for (auto operand : new_op.getOperands()) {
            if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(operand.getType())) {
                max_size = std::max(max_size, operandTy.getCol());
            } else if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(operand.getType())) {
                max_size = std::max(max_size, operandTy.getSize());
            } else if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherType>(operand.getType())) {
                // scalar-type input that will be converted
                if (auto loadOp = operand.template getDefiningOp<fhe::LoadOp>()) {
                    if (mlir::isa<fhe::LWECipherVectorType>(loadOp.getMemref().getType())) {
                        auto loadTy = mlir::dyn_cast<fhe::LWECipherVectorType>(loadOp.getMemref().getType());
                        max_size = std::max(max_size, loadTy.getSize());
                    } else if (mlir::isa<fhe::LWECipherMatrixType>(loadOp.getMemref().getType())) {
                        auto loadTy = mlir::dyn_cast<fhe::LWECipherMatrixType>(loadOp.getMemref().getType());
                        max_size = std::max(max_size, loadTy.getCol());
                    }
                }
            }
        }

        // convert the new op all operands from scalar to batched
        for (auto it = new_op->operand_begin(); it != new_op->operand_end(); ++it) {
            if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>((*it).getType())) {
                // Check if it needs to be resized
                if (operandTy.getCol() < max_size) {
                    auto resizeTy = fhe::LWECipherMatrixType::get(rewriter.getContext(), operandTy.getPlaintextType(), 
                                                                  operandTy.getRow(), max_size);
                    auto resizeOp = rewriter.create<fhe::CastOp>(op.getLoc(), resizeTy, *it);
                    rewriter.replaceUsesWithIf((*it).getDefiningOp()->getResults(), {resizeOp},
                                               [&](OpOperand &operand) { return operand.getOwner() == new_op; });
                }
            } else if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>((*it).getType())) {
                // Check if it needs to be resized
                if (operandTy.getSize() < max_size) {
                    auto resizeTy = fhe::LWECipherVectorType::get(rewriter.getContext(), operandTy.getPlaintextType(), max_size);
                    auto resizeOp = rewriter.create<fhe::CastOp>(op.getLoc(), resizeTy, *it);
                    rewriter.replaceUsesWithIf((*it).getDefiningOp()->getResults(), {resizeOp},
                                               [&](OpOperand &operand) { return operand.getOwner() == new_op; });
                }
            } else if (auto operandTy = dyn_cast_or_null<fhe::LWECipherType>((*it).getType())) {
                // scalar-type input that needs to be converted
                if (fhe::LoadOp loadOp = (*it).template getDefiningOp<fhe::LoadOp>()) {
                    // Check if it needs to be resized
                    if (auto memTy = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(loadOp.getMemref().getType())) {
                        if (memTy.getCol() < max_size) {
                            auto resizeTy = fhe::LWECipherMatrixType::get(rewriter.getContext(), memTy.getPlaintextType(), 
                                                                          memTy.getRow(), max_size);
                            auto curInsertPt = rewriter.getInsertionPoint();
                            rewriter.setInsertionPoint(loadOp);
                            auto resizeOp = rewriter.create<fhe::CastOp>(loadOp.getLoc(), resizeTy, loadOp.getMemref());
                            auto resizeLoadOp = rewriter.create<fhe::LoadOp>(
                                                loadOp.getLoc(), loadOp.getType(), resizeOp, loadOp.getIndices());
                            rewriter.replaceUsesWithIf(loadOp, {resizeLoadOp}, [&](OpOperand &operand) {
                                return operand.getOwner() == new_op;
                            });
                            loadOp = resizeLoadOp;
                            rewriter.setInsertionPoint(&*curInsertPt);
                        }
                    } else if (auto memTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(loadOp.getMemref().getType())) {
                        if (memTy.getSize() < max_size) {
                            auto resizeTy = fhe::LWECipherVectorType::get(rewriter.getContext(), memTy.getPlaintextType(), max_size);
                            auto curInsertPt = rewriter.getInsertionPoint();
                            rewriter.setInsertionPoint(loadOp);
                            auto resizeOp = rewriter.create<fhe::CastOp>(loadOp.getLoc(), resizeTy, loadOp.getMemref());
                            auto resizeLoadOp = rewriter.create<fhe::LoadOp>(
                                                loadOp.getLoc(), loadOp.getType(), resizeOp, loadOp.getIndices());
                            rewriter.replaceUsesWithIf(loadOp, {resizeLoadOp}, [&](OpOperand &operand) {
                                return operand.getOwner() == new_op;
                            });
                            loadOp = resizeLoadOp;
                            rewriter.setInsertionPoint(&*curInsertPt);
                        }
                    }

                    // Instead of using the load operation, use a rotation operation instead.
                    assert(static_cast<int64_t>(loadOp.getIndices().size()) <= 2 && "LoadOp indices size > 2 not support");
                    int src_row, src_slot;
                    if (loadOp.getIndices().size() == 1) {
                        auto idx = getConstantIntValue(loadOp.getIndices()[0]);
                        assert(idx.has_value());
                        src_slot = idx.value();

                        // no other target slot defined yet, let's make this the target
                        // we'll rotate by zero, but that's later canonicalized to no-op anyway
                        if (target_slot == -1) {
                            target_slot = src_slot;   
                        }
                    } else if (loadOp.getIndices().size() == 2) {
                        auto row = getConstantIntValue(loadOp.getIndices()[0]);
                        auto idx = getConstantIntValue(loadOp.getIndices()[1]);
                        assert(row.has_value() && idx.has_value());
                        src_row = row.value();
                        src_slot = idx.value();

                        // no other target row && target slot defined yet, let's make this the target
                        // we'll rotate by zero, but that's later canonicalized to no-op anyway
                        if (target_row == -1) {
                            target_row = src_row;   
                        }
                        if (target_slot == -1) {
                            target_slot = src_slot;   
                        }
                    }           

                    // Here, target_slot is the slot index in the destination operand (e.g., store) 
                    // where the value from the source operation is placed, while i is the index in the source operand.
                    // calculate right shift rotate count.
                    auto shiftRightCnt = ((target_slot - src_slot + max_size) % max_size);
                    if (NegativeShiftRight) {
                        shiftRightCnt = -shiftRightCnt;
                    }
                    LLVM_DEBUG(llvm::dbgs() << "target_slot=" << target_slot << ",load index=" << src_slot 
                                            << ",max sizes=" << max_size << "rotate index=" << shiftRightCnt << "\n");
                
                    // new rotate op an replace uses
                    mlir::Value rotOp;
                    if (loadOp.getIndices().size() == 1) {
                        rotOp = rewriter.create<fhe::RotateOp>(loadOp.getLoc(), loadOp.getMemref().getType(), 
                                                               loadOp.getMemref(), shiftRightCnt);
                    } else if (loadOp.getIndices().size() == 2) {
                        rotOp = rewriter.create<fhe::RotateExOp>(loadOp.getLoc(), loadOp.getMemref().getType(), 
                                                               loadOp.getMemref(), target_row, shiftRightCnt);
                    }
                    rewriter.replaceUsesWithIf(loadOp, {rotOp}, [&](OpOperand &operand) { 
                        return operand.getOwner() == new_op; 
                    });
                } else {
                    mlir::emitWarning(new_op.getLoc())
                            << "While attempting to batch, encountered an unexpected non-batchable defining operation, batching is aborted.";
                    rewriter.eraseOp(new_op);
                    return success();
                }
            } else {
                // non-secret input, which we can always transform as needed, no action needed now
            }
        }

        // re-create the op to get correct type inference
        auto newer_op =  rewriter.create<OpType>(new_op.getLoc(), new_op->getOperands()[0].getType(), 
                                                 new_op->getOperands());
        rewriter.eraseOp(new_op);
        new_op = newer_op;

        // Now create a scalar again by creating an load op, preserving type constraints
        rewriter.setInsertionPointAfter(new_op);
        if (target_row == -1 || mlir::isa<fhe::LWECipherVectorType>(new_op.getResult().getType())) {
            auto indexValue = rewriter.create<arith::ConstantIndexOp>(op.getLoc(), target_slot);
            auto res_new_op = rewriter.create<fhe::LoadOp>(op.getLoc(), op.getType(), new_op.getResult(), mlir::ValueRange{indexValue});
            op->replaceAllUsesWith(res_new_op);
        } else {
            auto rowValue = rewriter.create<arith::ConstantIndexOp>(op.getLoc(), target_row);
            auto indexValue = rewriter.create<arith::ConstantIndexOp>(op.getLoc(), target_slot);
            auto res_new_op = rewriter.create<fhe::LoadOp>(op.getLoc(), op.getType(), new_op.getResult(), mlir::ValueRange{rowValue, indexValue});
            op->replaceAllUsesWith(res_new_op);
        }

        // Finally, remove the original op
        rewriter.eraseOp(op);
    }

    return success();
}

void BatchingPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<affine::AffineDialect, func::FuncDialect, scf::SCFDialect,
                    fhe::FHEDialect>();
}

void BatchingPass::runOnOperation() {
    IRRewriter rewriter(&getContext());
    auto type_converter = TypeConverter();

    type_converter.addConversion([&](Type t) {
        if (mlir::isa<fhe::LWECipherVectorType>(t)) {
            auto srcTy = mlir::cast<fhe::LWECipherVectorType>(t);
            auto size = srcTy.getSize();
            return std::optional<Type>(fhe::RLWECipherType::get(&getContext(), srcTy.getPlaintextType(), size));
        }
        return std::optional<Type>(t);
    });

    type_converter.addTargetMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<fhe::RLWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<fhe::LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        } 

        llvm::errs() << "[BatchingPass] Materialization(addTargetMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<fhe::RLWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<fhe::LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "[BatchingPass] Materialization(addArgumentMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<fhe::LWECipherVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<fhe::RLWECipherType>(srcTy))
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
        }

        llvm::errs() << "[BatchingPass] Materialization(addSourceMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    // If FHE::LoadOp is not found, return directly
    bool hasLoadOp = false;
    getOperation()->walk([&](Operation *op) {
        if (isa<fhe::LoadOp>(op)) {
            hasLoadOp = true;
            return WalkResult::interrupt();
        }

        return WalkResult::advance();
    });

    if (!hasLoadOp) {
        return;
    }

    // Batching many thousands of values into a single vector-like ciphertext.
    // Get the (default) block in the module's only region:
    auto &block = getOperation()->getRegion(0).getBlocks().front();
    for (auto funcOp : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        // We must translate in order of appearance for this to work, so we walk manually
        if (funcOp.walk([&](Operation *op) {
                if (auto subOp = llvm::dyn_cast_or_null<fhe::LWESubOp>(op)) {
                    if (batchArithOperation<fhe::LWESubOp>(rewriter, &getContext(), subOp).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (auto subOp = llvm::dyn_cast_or_null<fhe::LWESubPlainOp>(op)) {
                    if (batchArithOperation<fhe::LWESubPlainOp>(rewriter, &getContext(), subOp).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (auto addOp = llvm::dyn_cast_or_null<fhe::LWEAddOp>(op)) {
                    if (batchArithOperation<fhe::LWEAddOp>(rewriter, &getContext(), addOp).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (auto addOp = llvm::dyn_cast_or_null<fhe::LWEAddPlainOp>(op)) {
                    if (batchArithOperation<fhe::LWEAddPlainOp>(rewriter, &getContext(), addOp).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (auto mulOp = llvm::dyn_cast_or_null<fhe::LWEMulOp>(op)) {
                    if (batchArithOperation<fhe::LWEMulOp>(rewriter, &getContext(), mulOp).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (auto mulOp = llvm::dyn_cast_or_null<fhe::LWEMulPlainOp>(op)) {
                    if (batchArithOperation<fhe::LWEMulPlainOp>(rewriter, &getContext(), mulOp).failed()) {
                        return WalkResult::interrupt();
                    }
                }
                return WalkResult(success());
        }).wasInterrupted())
        signalPassFailure();
    }

    // RotateOp type from LWE to RLWE
    auto LWERotOpToRLWERotOp = [](IRRewriter &rewriter, MLIRContext *context, fhe::RotateOp op,
                                TypeConverter typeConverter) -> LogicalResult {
        rewriter.setInsertionPoint(op);
        auto destTy = typeConverter.convertType(op.getType());
        if (!destTy) {
            return failure();
        }

        auto operand = op.getOperand();
        auto operandDestTy = typeConverter.convertType(operand.getType());
        auto newOperand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
        assert(newOperand);
        auto rotIndex = op.getI();
        rewriter.replaceOpWithNewOp<fhe::RotateOp>(op, destTy, newOperand, rotIndex);
        return success();
    };

    for (auto funcOp : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        // We must translate in order of appearance for this to work, so we walk manually
        if (funcOp.walk([&](Operation *op) {
            if (auto rotOp = llvm::dyn_cast_or_null<fhe::RotateOp>(op)) {
                if (LWERotOpToRLWERotOp(rewriter, &getContext(), rotOp, type_converter).failed())
                    return WalkResult::interrupt();
            }
            return WalkResult(success());
        }).wasInterrupted()) {
            signalPassFailure();
        }
    }
}
