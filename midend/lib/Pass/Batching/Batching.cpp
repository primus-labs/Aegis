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

template <typename OpType>
LogicalResult batchArithOperation(IRRewriter &rewriter, MLIRContext *context, OpType op) {
    // We care only about ops that return scalars, assuming others are already "SIMD-compatible"
    if (mlir::isa<fhe::LWECipherType>(op.getType())) {
        int target_slot = -1; //(-1 => no target slot required)

        // We only search one level deep for now.
        for (auto u : op->getUsers()) {
            if (fhe::LoadOp loadOp = mlir::dyn_cast_or_null<fhe::LoadOp>(u)) {
                assert(static_cast<int64_t>(loadOp.getIndices().size()) == 1 && "LoadOp indices size > 1 not support");
                auto idx = getConstantIntValue(loadOp.getIndices()[0]);
                assert(idx.has_value());
                target_slot = idx.value();
                break;
            } else if (fhe::StoreOp storeOp = mlir::dyn_cast_or_null<fhe::StoreOp>(u)) {
                assert(static_cast<int64_t>(storeOp.getIndices().size()) == 1 && "StoreOp indices size > 1 not support");
                auto idx = getConstantIntValue(storeOp.getIndices()[0]);
                assert(idx.has_value());
                target_slot = idx.value();
                break;
            } else if (auto retOp = mlir::dyn_cast_or_null<func::ReturnOp>(u)) {
                if (mlir::isa<fhe::LWECipherType>(retOp->getOperandTypes().front())) {
                    // we eventually want this as a scalar, which means slot 0
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
                        assert(static_cast<int64_t>(loadOp.getIndices().size()) == 1 && "LoadOp indices size > 1 not support");
                        auto idx = getConstantIntValue(loadOp.getIndices()[0]);
                        assert(idx.has_value());
                        auto i = idx.value();
                        if (target_slot == -1) {
                            target_slot = i;
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
            if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(operand.getType())) {
                max_size = std::max(max_size, operandTy.getSize());
            } else if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherType>(operand.getType())) {
                // scalar-type input that will be converted
                if (auto loadOp = operand.template getDefiningOp<fhe::LoadOp>()) {
                    auto loadTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(loadOp.getMemref().getType());
                    assert(loadTy && "here, the fhe::LoadOp must be applied to LWECipherVector");
                    max_size = std::max(max_size, loadTy.getSize());
                }
            }
        }

        // convert the new op all operands from scalar to batched
        for (auto it = new_op->operand_begin(); it != new_op->operand_end(); ++it) {
            if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>((*it).getType())) {
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
                    if (auto memTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(loadOp.getMemref().getType())) {
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
                    assert(static_cast<int64_t>(loadOp.getIndices().size()) == 1 && "LoadOp indices size > 1 not support");
                    auto idx = getConstantIntValue(loadOp.getIndices()[0]);
                    assert(idx.has_value());
                    auto i = idx.value();

                    // no other target slot defined yet, let's make this the target
                    // we'll rotate by zero, but that's later canonicalized to no-op anyway
                    if (target_slot == -1) {
                        target_slot = i;   
                    }
                    auto rotOp = rewriter.create<fhe::RotateOp>(loadOp.getLoc(), loadOp.getMemref().getType(), 
                                                                loadOp.getMemref(), (target_slot - i + max_size) % max_size);
                    rewriter.replaceUsesWithIf(loadOp, {rotOp}, [&](OpOperand &operand) { 
                        return operand.getOwner() == new_op; 
                    });
                } else {
                    emitWarning(new_op.getLoc(),
                                "While attempting to batch, encountered an unexpected non-batchable defining operation for the secret operand.");
                    return failure();
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
        auto indexValue = rewriter.create<arith::ConstantIndexOp>(op.getLoc(), target_slot);
        auto res_new_op = rewriter.create<fhe::LoadOp>(op.getLoc(), op.getType(), new_op.getResult(), mlir::ValueRange{indexValue});
        op->replaceAllUsesWith(res_new_op);

        // Finally, remove the original op
        rewriter.eraseOp(op);
    }

    return success();
}

template <typename OpType>
LogicalResult batchLoadStoreOperation(IRRewriter &rewriter, MLIRContext *context, OpType op) {
    rewriter.setInsertionPoint(op);
    int target_slot = -1;

    if (mlir::isa<fhe::LoadOp>(op)) {
        /**********************************************************************************
        %7 = fhe.load(%6, %c0) : (!fhe.lweciphervec<16 x f64>, index) -> !fhe.lwecipher<f64>
        //fhe.store(%7, %arg1, %c0) : (!fhe.lwecipher<f64>, !fhe.lweciphervec<4 x f64>, index)
        =>
        %rotated = fhe.rotate(%6, 0) 
        %mask = arith.constant dense<[1.0, 0.0, ..., 0.0]> : tensor<16xf64>
        %7 = fhe.mulplain(%rotated, %mask) : !fhe.lweciphervec<16 x f64>
        ************************************************************************************/
        assert(static_cast<int64_t>(op.getIndices().size()) == 1 && "LoadOp indices size > 1 not support");
        auto idx = getConstantIntValue(op.getIndices()[0]);
        assert(idx.has_value());
        target_slot = idx.value();

        // find the maximum size of LoadOp memref operand
        int max_size = -1;
        for (auto operand : op.getOperands()) {
            if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(operand.getType())) {
                max_size = std::max(max_size, operandTy.getSize());
            }
        }

        // Create rotation amount (negative index to bring element to position 0)
        auto rotateAmount = -target_slot % max_size;

         // Apply rotation operation
        auto rotateOp = rewriter.create<fhe::RotateOp>(op.getLoc(), op.getMemref().getType(),
                                                       op.getMemref(), rotateAmount);

        // Create selection mask [1,0,0,...]
        mlir::Type elementType = rewriter.getI32Type();
        auto arrayType = mlir::VectorType::get({max_size}, elementType);
        SmallVector<int32_t> maskValues(max_size, 0);
        maskValues[0] = 1;
        llvm::ArrayRef<int32_t> valuesRef(maskValues); 
        mlir::DenseElementsAttr denseAttr = mlir::DenseElementsAttr::get(arrayType, valuesRef);
        auto maskOp = rewriter.create<arith::ConstantOp>(op.getLoc(), denseAttr);

        // Apply element-wise multiplication to select the first element
        llvm::SmallVector<Value> operands;
        operands.push_back(rotateOp);
        operands.push_back(maskOp);
        rewriter.replaceOpWithNewOp<fhe::LWEMulPlainOp>(op, rotateOp.getType(), operands);
    } else if (mlir::isa<fhe::StoreOp>(op)) {
        /***********************************************************************************
        //%7 = fhe.load(%6, %c0) : (!fhe.lweciphervec<16 x f64>, index) -> !fhe.lwecipher<f64>
        fhe.store(%7, %arg1, %c0) : (!fhe.lwecipher<f64>, !fhe.lweciphervec<4 x f64>, index)
        =>
        %inverted_mask = arith.constant dense<[0,1,1,1...]> : tensor<16xf64> 
        %existing = fhe.mulplain %arg1, %inverted_mask : !fhe.lweciphervec<16xf64>
        %new_val = fhe.add %existing, %7 : !fhe.lweciphervec<16xf64>
        fhe.copy %new_val, %arg1 :!fhe.lweciphervec<16xf64> -> !fhe.lweciphervec<16xf64>
        ***********************************************************************************/
        auto storeOp = llvm::cast<fhe::StoreOp>(op);
        assert(static_cast<int64_t>(storeOp.getIndices().size()) == 1 && "StoreOp indices size > 1 not support");
        auto idx = getConstantIntValue(storeOp.getIndices()[0]);
        assert(idx.has_value());
        target_slot = idx.value();

        // find the maximum size of StoreOp memref operand
        int max_size = -1;
        for (auto operand : storeOp.getOperands()) {
            if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(operand.getType())) {
                max_size = std::max(max_size, operandTy.getSize());
            }
        }

        for (auto curOperand : storeOp->getOperands()) {
            if (fhe::LWEMulPlainOp mulplainOp = curOperand.template getDefiningOp<fhe::LWEMulPlainOp>()) {
                for (auto operand : mulplainOp.getOperands()) {
                    if (auto constOp = operand.getDefiningOp<arith::ConstantOp>()) {
                        Attribute valueAttr = constOp.getValue();
                        if (auto denseAttr = mlir::dyn_cast<mlir::DenseElementsAttr>(valueAttr)) {
                            // Convert to DenseElementsAttr(vector/tensor)
                            auto constVals = denseAttr.getValues<int32_t>();
                            size_t oneValIdx = 0;
                            for (auto i = 0; i < constVals.size(); i++) {
                                if (constVals[i] == 1) {
                                    oneValIdx = i;
                                    break;
                                }
                            }

                            // Invert the mast value and create constOp
                            mlir::Type elementType = rewriter.getI32Type();
                            auto arrayType = mlir::VectorType::get({max_size}, elementType);
                            SmallVector<int32_t> invertMask(max_size, 1);
                            invertMask[oneValIdx] = 0;
                            llvm::ArrayRef<int32_t> invertMaskRef(invertMask); 
                            mlir::DenseElementsAttr denseAttr2 = mlir::DenseElementsAttr::get(arrayType, invertMaskRef);
                            auto invertMaskOp = rewriter.create<arith::ConstantOp>(storeOp.getLoc(), denseAttr2);    

                            // Create fhe.mulplain
                            auto mulVal = rewriter.create<fhe::LWEMulPlainOp>(storeOp.getLoc(), storeOp.getMemref().getType(),
                                                                              ValueRange({storeOp.getMemref(), invertMaskOp}));

                            // Combine new and existing values
                            auto addOp = rewriter.create<fhe::LWEAddOp>(storeOp.getLoc(), mulVal.getType(),
                                                                         ValueRange({mulVal, storeOp.getValueToStore()}));

                            // copy the new values(fhe::LWEAddOp) to StoreOp memref value
                            auto result = rewriter.create<fhe::CopyOp>(storeOp.getLoc(), addOp, storeOp.getMemref());

                            // Replace the store operation with fhe::CopyOp
                            rewriter.replaceOp(op, result);
                            break;               
                        }
                    }
                }
                break;
            }
        }
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
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<fhe::LWECipherVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<fhe::RLWECipherType>(srcTy))
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
        } 
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

    // After the batch operation, exist some redundant load & store op, we mush to delete it.
    // If there are redundant load & store operations, they can affect the subsequent ​​batchLoadStoreOperation​​ optimization.
    PassManager nestedPM(&getContext());
    nestedPM.addPass(createCanonicalizerPass());
    nestedPM.addPass(createCSEPass());
    if (failed(nestedPM.run(getOperation()))) {
        signalPassFailure();
    }

    // After the batch operation is completed, load & store pairs may be reserved. At this time, 
    // the load & store pairs need to be optimized and converted into rotate and corresponding arith operations.
    for (auto funcOp : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        if (funcOp.walk([&](Operation *op) {
            if (auto loadOp = llvm::dyn_cast_or_null<fhe::LoadOp>(op)) {
                    if (batchLoadStoreOperation<fhe::LoadOp>(rewriter, &getContext(), loadOp).failed()) {
                        return WalkResult::interrupt();
                    }
            } else if (auto storeOp = llvm::dyn_cast_or_null<fhe::StoreOp>(op)) {
                if (batchLoadStoreOperation<fhe::StoreOp>(rewriter, &getContext(), storeOp).failed()) {
                    return WalkResult::interrupt();
                }
            }
            return WalkResult(success());
        }).wasInterrupted())
        signalPassFailure();
    }

    // RotateOp type from LWE to RLWE
    auto RotOpToRLWEOp = [](IRRewriter &rewriter, MLIRContext *context, fhe::RotateOp op,
                            TypeConverter typeConverter) -> LogicalResult {
        rewriter.setInsertionPoint(op);
        auto destTy = typeConverter.convertType(op.getType());
        if (!destTy) {
            return failure();
        }

        auto operand = op.getOperand();
        auto operandDestTy = typeConverter.convertType(operand.getType());
        auto newOperand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
        auto rotIndex = op.getI();
        rewriter.replaceOpWithNewOp<fhe::RotateOp>(op, destTy, newOperand, rotIndex);
        return success();
    };

    for (auto funcOp : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        // We must translate in order of appearance for this to work, so we walk manually
        if (funcOp.walk([&](Operation *op) {
            if (auto rotOp = llvm::dyn_cast_or_null<fhe::RotateOp>(op)) {
                if (RotOpToRLWEOp(rewriter, &getContext(), rotOp, type_converter).failed())
                    return WalkResult::interrupt();
            }
            return WalkResult(success());
        }).wasInterrupted()) {
            signalPassFailure();
        }
    }
}