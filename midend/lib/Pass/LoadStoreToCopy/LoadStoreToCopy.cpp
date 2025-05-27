#include <optional> 
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Utils/IndexingUtils.h"
#include "mlir/Dialect/Utils/StaticValueUtils.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Sequence.h"
#include "Pass/LoadStoreToCopy/LoadStoreToCopy.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "loadstore-to-copy"

using namespace mlir;
using namespace aegis;


struct LoadStoreToCopyPattern : public OpRewritePattern<fhe::StoreOp> {
    LoadStoreToCopyPattern(MLIRContext *context) : OpRewritePattern<fhe::StoreOp>(context, /*benefit=*/1) {}

    LogicalResult matchAndRewrite(fhe::StoreOp storeOp, PatternRewriter &rewriter) const override {
        // Verify store value comes from load operation
        auto loadOp = storeOp.getValueToStore().getDefiningOp<fhe::LoadOp>();
        if (!loadOp) {
            LLVM_DEBUG(llvm::dbgs() << "Store value not from LoadOp\n");
            return failure();
        }

        // Get source and destination memrefs
        Value srcMem = loadOp.getMemref();
        Value destMem = storeOp.getMemref();

        // Type verification (vector or matrix)
        auto srcVecType = mlir::dyn_cast<fhe::LWECipherVectorType>(srcMem.getType());
        auto srcMatType = mlir::dyn_cast<fhe::LWECipherMatrixType>(srcMem.getType());
        auto destVecType = mlir::dyn_cast<fhe::LWECipherVectorType>(destMem.getType());
        auto destMatType = mlir::dyn_cast<fhe::LWECipherMatrixType>(destMem.getType());

        // Type compatibility checks
        bool isMatrix = (srcMatType != nullptr);
        if ((isMatrix && !destMatType) || (!isMatrix && !destVecType)) {
            LLVM_DEBUG({
                llvm::dbgs() << "Type mismatch\n";
                llvm::dbgs() << "  Source: " << srcMem.getType() << "\n";
                llvm::dbgs() << "  Dest: " << destMem.getType() << "\n";
            });
            return failure();
        }
        if (srcVecType) {
            if (srcVecType.getSize() != destVecType.getSize())
                return failure();
        } else {
            if (srcMatType.getRow() != destMatType.getRow() || srcMatType.getCol() != destMatType.getCol()) {
                return failure();
            }
        }

         // Get dimension information
        const int64_t totalElements = isMatrix ? srcMatType.getRow() * srcMatType.getCol() : srcVecType.getSize();
        const int64_t cols = isMatrix ? srcMatType.getCol() : 0;

        // Validate index count
        const size_t requiredIndices = isMatrix ? 2 : 1;
        if (storeOp.getIndices().size() != requiredIndices) {
            LLVM_DEBUG(llvm::dbgs() << "Invalid index count: " << storeOp.getIndices().size() << "\n");
            return failure();
        }

        // Collect current operation's linear index
        auto getStoreIndex = [&](fhe::StoreOp candidateStore) -> std::optional<int64_t> {
            if (isMatrix) {
                if (candidateStore.getIndices().size() != 2) {
                    return std::nullopt;
                }
                auto rowIdx = candidateStore.getIndices()[0].getDefiningOp<arith::ConstantIndexOp>();
                auto colIdx = candidateStore.getIndices()[1].getDefiningOp<arith::ConstantIndexOp>();
                if (!rowIdx || !colIdx) {
                    return std::nullopt;
                }
                const int64_t i = rowIdx.value();
                const int64_t j = colIdx.value();
                if (i < 0 || i >= srcMatType.getRow() || j < 0 || j >= cols) {
                    LLVM_DEBUG(llvm::dbgs() << " Matrix index out of range: [" << i << ", " << j << "]\n");
                    return std::nullopt;
                }
                return i * cols + j;
            } else {
                if (candidateStore.getIndices().size() != 1) {
                    return std::nullopt;
                }
                auto idx = candidateStore.getIndices()[0].getDefiningOp<arith::ConstantIndexOp>();
                if (!idx) {
                    return std::nullopt;
                }
                const int64_t index = idx.value();
                if (index < 0 || index >= srcVecType.getSize()) {
                    LLVM_DEBUG(llvm::dbgs() << "Vector index out of range: " << index << "\n");
                    return std::nullopt;
                }
                return index;
            }
        };

        std::optional<int64_t> currentIndex = getStoreIndex(storeOp);
        if (!currentIndex.has_value()) {
            return failure();
        }

        // Collect related operations in the same block
        DenseMap<int64_t, std::pair<fhe::StoreOp, fhe::LoadOp>> opsMap;
        for (Operation &op : *storeOp->getBlock()) {
            if (auto candidateStore = dyn_cast<fhe::StoreOp>(&op)) {
                // Verify destination memref
                if (candidateStore.getMemref() != destMem) {
                    continue;
                }

                // Verify load operation source
                auto candidateLoad = candidateStore.getValueToStore().getDefiningOp<fhe::LoadOp>();
                if (!candidateLoad || candidateLoad.getMemref() != srcMem) {
                    continue;
                }

                // Calculate candidate's linear index
                std::optional<int64_t> idx = getStoreIndex(candidateStore);
                if (!idx.has_value() || idx.value() < 0 || idx.value() >= totalElements) {
                    LLVM_DEBUG(llvm::dbgs() << "Invalid candidate index\n");
                    continue;
                }

                opsMap.try_emplace(idx.value(), candidateStore, candidateLoad);
            }
        }

        // Verify complete coverage
         if (opsMap.size() != totalElements) {
            LLVM_DEBUG(llvm::dbgs() << "Incomplete coverage: " << opsMap.size() << "/" << totalElements << "\n");
            return failure();
        }
        for (int64_t i = 0; i < totalElements; ++i) {
            if (!opsMap.count({i})) {
                return failure();
            }
        }
        
        // Safety checks between first store and insertion point
        // Operation* lastStore = opsMap[totalElements - 1].first;
        Operation* lastStore = nullptr;
        for (auto &entry : opsMap) {
            Operation* candidate = entry.second.first;
            if (!lastStore || lastStore->isBeforeInBlock(candidate)) {
                lastStore = candidate;
            }
        }
        if (!lastStore) {
            LLVM_DEBUG(llvm::dbgs() << "Failed to find last store operation\n");
            return failure();
        }
        Operation* copyInsertPoint = lastStore->getNextNode();
        bool hasDangerousOp = false;

        // Get all collected operations for quick lookup
        DenseSet<Operation*> collectedOps;
        for (auto &entry : opsMap) {
            collectedOps.insert(entry.second.first);
            collectedOps.insert(entry.second.second);
        }

        for (Operation& op : *storeOp->getBlock()) {
            // Stop at insertion point
            if (&op == copyInsertPoint) {
                break;
            }

            // Skip collected operations
            if (collectedOps.contains(&op)) {
                continue;
            }

            // Check for dangerous memref usages
            auto checkOperands = [&](Operation* op) {
                for (Value operand : op->getOperands()) {
                    if (operand == srcMem || operand == destMem) {
                        hasDangerousOp = true;
                        return;
                    }
                }
            };

            checkOperands(&op);
            if (hasDangerousOp) {
                break;
            }

            if (auto load = mlir::dyn_cast<fhe::LoadOp>(&op)) {
                if (load.getMemref() == srcMem || load.getMemref() == destMem) {
                    hasDangerousOp = true;
                    break;
                }
            }
            if (auto store = mlir::dyn_cast<fhe::StoreOp>(&op)) {
                if (store.getMemref() == destMem) {
                    hasDangerousOp = true;
                    break;
                }
            }
        }

        if (hasDangerousOp) {
            return failure();
        }

        // Create copy operation
        rewriter.setInsertionPointAfter(lastStore);
        rewriter.create<fhe::CopyOp>(storeOp.getLoc(), srcMem, destMem);

        // Erase old operations in reverse order
        SmallVector<Operation*> toErase;
        for (auto &entry : opsMap) {
            toErase.push_back(entry.second.first);
            toErase.push_back(entry.second.second);
        }
        for (auto it = toErase.rbegin(); it != toErase.rend(); ++it) {
            rewriter.eraseOp(*it);
        }

        return success();
    }
};

template <typename OpType>
LogicalResult batchLoadStoreOperation(IRRewriter &rewriter, MLIRContext *context, OpType op) {
    rewriter.setInsertionPoint(op);
    int target_row = -1;
    int target_slot = -1;

    if (mlir::isa<fhe::LoadOp>(op)) {
        auto loadOp = llvm::cast<fhe::LoadOp>(op);
        assert(static_cast<int64_t>(loadOp.getIndices().size()) <= 2 && "LoadOp indices size > 2 not support");
        if (loadOp.getIndices().size() == 1) {
            /**********************************************************************************
            // incorrent
            %7 = fhe.load(%6, %c0) : (!fhe.lweciphervec<16 x f64>, index) -> !fhe.lwecipher<f64>
            =>
            It has already been rotated and has the correct target slot, so there is no need to rotate it again.
            //%rotated = fhe.rotate(%6, 0) 
            //%mask = arith.constant dense<[1.0, 0.0, ..., 0.0]> : tensor<16xf64>
            //%7 = fhe.mulplain(%rotated, %mask) : !fhe.lweciphervec<16 x f64>
            ------------------------------------------------------------------------------------
            // correct
            %7 = fhe.load(%6, %c0) : (!fhe.lweciphervec<16 x f64>, index) -> !fhe.lwecipher<f64>
            =>
            %mask = arith.constant dense<[1.0, 0.0, ..., 0.0]>
            %new = fhe.mulplain(%6, %mask);
            ************************************************************************************/
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

            /*
            // Create rotation amount (bring element to position 0)
            auto rotateAmount = ((-target_slot + max_size) % max_size);
            if (kNegativeShiftRight) {
                rotateAmount = -rotateAmount;
            }
            LLVM_DEBUG(llvm::dbgs() << "target slot=" << target_slot << ", max size=" << max_size << ", (shift right to 0 slot)real rotate value=" << rotateAmount << "\n");

            // Apply rotation operation
            auto rotateOp = rewriter.create<fhe::RotateOp>(op.getLoc(), op.getMemref().getType(),
                                                        op.getMemref(), rotateAmount);

            // Create selection mask [1,0,0,...]
            mlir::Type elementType = rewriter.getI32Type();
            auto arrayType = mlir::VectorType::get({max_size}, elementType);
            SmallVector<int32_t> maskValues(max_size, 0);
            maskValues[target_slot] = 1;
            llvm::ArrayRef<int32_t> valuesRef(maskValues); 
            mlir::DenseElementsAttr denseAttr = mlir::DenseElementsAttr::get(arrayType, valuesRef);
            auto maskOp = rewriter.create<arith::ConstantOp>(op.getLoc(), denseAttr);

            // Apply element-wise multiplication to select the first element
            llvm::SmallVector<Value> operands;
            operands.push_back(rotateOp);
            operands.push_back(maskOp);
            rewriter.replaceOpWithNewOp<fhe::LWEMulPlainOp>(op, rotateOp.getType(), operands);
            */

            // Create selection mask [0..,1[target_slot],...,0]
            mlir::Type elementType = rewriter.getI32Type();
            auto arrayType = mlir::VectorType::get({max_size}, elementType);
            SmallVector<int32_t> maskValues(max_size, 0);
            maskValues[target_slot] = 1;
            llvm::ArrayRef<int32_t> valuesRef(maskValues); 
            mlir::DenseElementsAttr denseAttr = mlir::DenseElementsAttr::get(arrayType, valuesRef);
            auto maskOp = rewriter.create<arith::ConstantOp>(op.getLoc(), denseAttr);

            // If the result of a load operation is uniquely used by a return, 
            // then rotate the elements based on target_slot to move the target element to the first position. 
            // Then insert a cast operation to convert !fhe.lweciphervec<N x f64> to !fhe.lweciphervec<f64> and return it.
            auto isOnlyReturnUse = [](auto result) -> bool {
                if (!result.hasOneUse()) {
                    return false;
                }
                auto user = *result.user_begin();
                if (isa<func::ReturnOp>(user)) {
                    return true;
                } else {
                    return false;
                }
            };
            
            if (!isOnlyReturnUse(loadOp.getResult())) {
                // Apply element-wise multiplication to select the first element
                rewriter.replaceOpWithNewOp<fhe::LWEMulPlainOp>(op, op.getMemref().getType(), ValueRange({op.getMemref(), maskOp}));
            } else { 
                // create multiplication op
                auto mulOp = rewriter.create<fhe::LWEMulPlainOp>(op.getLoc(), op.getMemref().getType(), ValueRange({op.getMemref(), maskOp}));
                
                // rotate the elements based on target_slot to move the target element to the first position.
                auto rotOp = rewriter.create<fhe::RotateOp>(mulOp.getLoc(), mulOp.getType(), mulOp, target_slot);

                // use cast to convert !fhe.lweciphervec<N x f64> to !fhe.lweciphervec<f64>
                rewriter.replaceOpWithNewOp<fhe::CastOp>(op, loadOp.getType(), rotOp);
            }
        } else if (loadOp.getIndices().size() == 2) {
            /**********************************************************************************
            %7 = fhe.load(%6, %c0, %c1) : (!fhe.lweciphervec<4x16xf64>, index) -> !fhe.lwecipher<f64>
            =>
            %7 = fhe.vload(%6, %c0) : (!fhe.lweciphervec<4x16xf64>, index) -> !fhe.lwecipher<16xf64>
            %mask = arith.constant dense<[1.0, 0.0, ..., 0.0]>
            %new = fhe.mulplain(%7, %mask);
            ************************************************************************************/
            auto row = getConstantIntValue(op.getIndices()[0]);
            auto idx = getConstantIntValue(op.getIndices()[1]);
            assert(row.has_value() && idx.has_value());
            target_row = row.value();
            target_slot = idx.value();

            // find the maximum col size of LoadOp memref operand
            Type ptType;
            int max_size = -1;
            for (auto operand : op.getOperands()) {
                if (auto operandTy = mlir::dyn_cast<fhe::LWECipherMatrixType>(operand.getType())) {
                    ptType = operandTy.getPlaintextType();
                    max_size = std::max(max_size, operandTy.getCol());
                }
            }

            // Create fhe.vload op
            mlir::Value rowVal = rewriter.create<arith::ConstantOp>(op.getLoc(), rewriter.getIndexAttr(target_row));
            auto vecType = fhe::LWECipherVectorType::get(rewriter.getContext(), ptType, max_size);
            auto vloadVal = rewriter.create<fhe::VloadOp>(op.getLoc(), vecType, op.getMemref(), rowVal);

            // Create selection mask [0..,1[target_slot],...,0]
            mlir::Type elementType = rewriter.getI32Type();
            auto arrayType = mlir::VectorType::get({max_size}, elementType);
            SmallVector<int32_t> maskValues(max_size, 0);
            maskValues[target_slot] = 1;
            llvm::ArrayRef<int32_t> valuesRef(maskValues); 
            mlir::DenseElementsAttr denseAttr = mlir::DenseElementsAttr::get(arrayType, valuesRef);
            auto maskOp = rewriter.create<arith::ConstantOp>(op.getLoc(), denseAttr);

            // If the result of a load operation is uniquely used by a return, 
            // then rotate the elements based on target_slot to move the target element to the first position. 
            // Then insert a cast operation to convert !fhe.lweciphervec<N x f64> to !fhe.lweciphervec<f64> and return it.
            auto isOnlyReturnUse = [](auto result) -> bool {
                if (!result.hasOneUse()) {
                    return false;
                }
                auto user = *result.user_begin();
                if (isa<func::ReturnOp>(user)) {
                    return true;
                } else {
                    return false;
                }
            };
            
            if (!isOnlyReturnUse(loadOp.getResult())) {
                // Apply element-wise multiplication to select the first element
                rewriter.replaceOpWithNewOp<fhe::LWEMulPlainOp>(op, vecType, ValueRange({vloadVal, maskOp}));
            } else { 
                // create multiplication op
                auto mulOp = rewriter.create<fhe::LWEMulPlainOp>(op.getLoc(), vecType, ValueRange({vloadVal, maskOp}));
                
                // rotate the elements based on target_slot to move the target element to the first position.
                auto rotOp = rewriter.create<fhe::RotateOp>(mulOp.getLoc(), mulOp.getType(), mulOp, target_slot);

                // use cast to convert !fhe.lweciphervec<N x f64> to !fhe.lweciphervec<f64>
                rewriter.replaceOpWithNewOp<fhe::CastOp>(op, loadOp.getType(), rotOp);
            }
        }
    } else if (mlir::isa<fhe::StoreOp>(op)) {
        auto storeOp = llvm::cast<fhe::StoreOp>(op);
        assert(static_cast<int64_t>(storeOp.getIndices().size()) <= 2 && "StoreOp indices size > 2 not support");
        if (storeOp.getIndices().size() == 1) {
            /***********************************************************************************
            //%7 = fhe.load(%6, %c0) : (!fhe.lweciphervec<16 x f64>, index) -> !fhe.lwecipher<f64>
            fhe.store(%7, %arg1, %c0) : (!fhe.lwecipher<f64>, !fhe.lweciphervec<4 x f64>, index)
            =>
            %inverted_mask = arith.constant dense<[0,1,1,1...]> : tensor<16xf64> 
            %existing = fhe.mulplain %arg1, %inverted_mask : !fhe.lweciphervec<16xf64>
            %new_val = fhe.add %existing, %7 : !fhe.lweciphervec<16xf64>
            fhe.copy %new_val, %arg1 :!fhe.lweciphervec<16xf64> -> !fhe.lweciphervec<16xf64>
            ***********************************************************************************/
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

                                // if target_slot != oneValIdx, we need calculate the rotate index,
                                // then insert rotate op after the lwemulplain op
                                bool bNeedRot = false;
                                mlir::Value rotOp;
                                if (target_slot != oneValIdx) {
                                    bNeedRot = true;
                                    auto shiftRightCnt = ((target_slot - oneValIdx + max_size) % max_size);
                                    if (kNegativeShiftRight) {
                                        shiftRightCnt = -shiftRightCnt;
                                    }
                                    rotOp = rewriter.create<fhe::RotateOp>(mulplainOp.getLoc(), mulplainOp.getType(), 
                                                                           mulplainOp, shiftRightCnt);
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
                                mlir::Value addOp;
                                if (bNeedRot) {
                                    addOp = rewriter.create<fhe::LWEAddOp>(storeOp.getLoc(), mulVal.getType(),
                                                                            ValueRange({mulVal, rotOp}));
                                } else {
                                    addOp = rewriter.create<fhe::LWEAddOp>(storeOp.getLoc(), mulVal.getType(),
                                                                            ValueRange({mulVal, storeOp.getValueToStore()}));
                                }

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
        } else if (storeOp.getIndices().size() == 2) {
            /***********************************************************************************
            //%7 = fhe.load(%6, %c0, %c1) : (!fhe.lweciphervec<4x16xf64>, index) -> !fhe.lwecipher<f64>
            fhe.store(%7, %arg1, %c0, %c2) : (!fhe.lwecipher<f64>, !fhe.lweciphervec<4x16xf64>, index)
            =>
            %row_arg1 = fhe.vload %arg1, %c0 : !fhe.lweciphervec<16xf64>
            %inverted_mask = arith.constant dense<[0,1,1,1...]> : tensor<16xf64> 
            %existing = fhe.mulplain %row_arg1, %inverted_mask : !fhe.lweciphervec<16xf64>
            %new_val = fhe.add %existing, %7 : !fhe.lweciphervec<16xf64>
            fhe.vstore %new_val, %arg1, %c0 :!fhe.lweciphervec<16xf64> -> !fhe.lweciphervec<4x16xf64>
            ***********************************************************************************/
            auto row = getConstantIntValue(storeOp.getIndices()[0]);
            auto idx = getConstantIntValue(storeOp.getIndices()[1]);
            assert(row.has_value() && idx.has_value());
            target_row = row.value();
            target_slot = idx.value();

            // find the maximum size of StoreOp memref operand
            Type ptType;
            int max_size = -1;
            for (auto operand : storeOp.getOperands()) {
                if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(operand.getType())) {
                    ptType = operandTy.getPlaintextType();
                    max_size = std::max(max_size, operandTy.getCol());
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

                                // if target_slot != oneValIdx, we need calculate the rotate index,
                                // then insert rotate op after the lwemulplain op
                                bool bNeedRot = false;
                                mlir::Value rotOp;
                                if (target_slot != oneValIdx) {
                                    bNeedRot = true;
                                    auto shiftRightCnt = ((target_slot - oneValIdx + max_size) % max_size);
                                    if (kNegativeShiftRight) {
                                        shiftRightCnt = -shiftRightCnt;
                                    }
                                    rotOp = rewriter.create<fhe::RotateOp>(mulplainOp.getLoc(), mulplainOp.getType(), 
                                                                           mulplainOp, shiftRightCnt);
                                }

                                // Create fhe.vload op
                                mlir::Value rowVal = rewriter.create<arith::ConstantOp>(storeOp.getLoc(), rewriter.getIndexAttr(target_row));
                                auto vecType = fhe::LWECipherVectorType::get(rewriter.getContext(), ptType, max_size);
                                auto vloadVal = rewriter.create<fhe::VloadOp>(storeOp.getLoc(), vecType, storeOp.getMemref(), rowVal);

                                // Invert the mast value and create constOp
                                mlir::Type elementType = rewriter.getI32Type();
                                auto arrayType = mlir::VectorType::get({max_size}, elementType);
                                SmallVector<int32_t> invertMask(max_size, 1);
                                invertMask[oneValIdx] = 0;
                                llvm::ArrayRef<int32_t> invertMaskRef(invertMask); 
                                mlir::DenseElementsAttr denseAttr2 = mlir::DenseElementsAttr::get(arrayType, invertMaskRef);
                                auto invertMaskOp = rewriter.create<arith::ConstantOp>(storeOp.getLoc(), denseAttr2);    

                                // Create fhe.mulplain
                                auto mulVal = rewriter.create<fhe::LWEMulPlainOp>(storeOp.getLoc(), vecType,
                                                                                ValueRange({vloadVal, invertMaskOp}));

                                // Combine new and existing values
                                mlir::Value addOp;
                                if (bNeedRot) {
                                    addOp = rewriter.create<fhe::LWEAddOp>(storeOp.getLoc(), mulVal.getType(),
                                                                           ValueRange({mulVal, rotOp}));
                                } else {
                                    addOp = rewriter.create<fhe::LWEAddOp>(storeOp.getLoc(), mulVal.getType(),
                                                                           ValueRange({mulVal, storeOp.getValueToStore()}));
                                }

                                // store the new values(fhe::LWEAddOp) to matrix memref value
                                auto result = rewriter.create<fhe::VstoreOp>(storeOp.getLoc(), addOp, storeOp.getMemref(), rowVal);

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
    }
    return success();
}


void LoadStoreToCopyPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, func::FuncDialect>();
}


void LoadStoreToCopyPass::runOnOperation() {
    // LoadStore seq to copy
    RewritePatternSet patterns(&getContext());
    patterns.add<LoadStoreToCopyPattern>(&getContext()); 
    if (failed(applyPatternsAndFoldGreedily(getOperation(), std::move(patterns)))) {
      signalPassFailure();
    }

    // After the batch operation is completed, load & store pairs may be reserved. At this time, 
    // the load & store pairs need to be optimized and converted into rotate and corresponding arith operations.
    IRRewriter rewriter(&getContext());
    auto &block = getOperation()->getRegion(0).getBlocks().front();
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
}