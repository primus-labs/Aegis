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

        // Check index is constant
        if (storeOp.getIndices().size() > 1) {
            LLVM_DEBUG(llvm::dbgs() << "StoreOp Indices size > 1\n");
            return failure();
        }
        auto constIndex = storeOp.getIndices()[0].getDefiningOp<arith::ConstantIndexOp>();
        if (!constIndex) {
            LLVM_DEBUG(llvm::dbgs() << "Non-constant index\n");
            return failure();
        }

        // Validate vector types
        Value sourceVec = loadOp.getMemref();
        Value destVec = storeOp.getMemref();
        auto sourceType = mlir::dyn_cast<fhe::LWECipherVectorType>(sourceVec.getType());
        auto destType = mlir::dyn_cast<fhe::LWECipherVectorType>(destVec.getType());
        if (!sourceType || !destType || sourceType != destType) {
            LLVM_DEBUG({
                llvm::dbgs() << "Type mismatch\n";
                if (!sourceType) llvm::dbgs() << "  Source is not LWECipherVecType\n";
                if (!destType) llvm::dbgs() << "  Dest is not LWECipherVecType\n";
                if (sourceType != destType) 
                    llvm::dbgs() << "  Type mismatch: " << sourceType << " vs " << destType << "\n";
            });
            return failure();
        }

        // Check source type size equal the dest type size
        const int64_t srcVecSize  = sourceType.getSize();
        const int64_t destVecSize = destType.getSize();
        if (srcVecSize != destVecSize) {
            LLVM_DEBUG(llvm::dbgs() << "source vector size not equal dest vector size\n");
            return failure();
        }

        // Collect all related operations
        DenseMap<int64_t, std::pair<fhe::StoreOp, fhe::LoadOp>> opsMap;
        for (Operation &op : *storeOp->getBlock()) {
            if (auto candidateStore = dyn_cast<fhe::StoreOp>(&op)) {
                // Must use same destination vector
                if (candidateStore.getMemref() != destVec) {
                    continue;
                }

                // Must load from same source vector
                auto candidateLoad = candidateStore.getValueToStore().getDefiningOp<fhe::LoadOp>();
                if (!candidateLoad || candidateLoad.getMemref() != sourceVec) {
                    continue;
                }

                // Must use constant index in valid range
                if (candidateStore.getIndices().size() > 1) {
                    LLVM_DEBUG(llvm::dbgs() << "StoreOp Indices size > 1\n");
                    continue;
                }
                auto idx = candidateStore.getIndices()[0].getDefiningOp<arith::ConstantIndexOp>();
                if (!idx || idx.value() < 0 || idx.value() >= srcVecSize) {
                    continue;
                }

                opsMap.try_emplace(idx.value(), candidateStore, candidateLoad);
            }
        }

        // Step 5: Verify complete index coverage
        if (opsMap.size() != srcVecSize) {
            LLVM_DEBUG(llvm::dbgs() << "Index coverage incomplete ("
                                    << opsMap.size() << "/" << srcVecSize << ")\n");
            return failure();
        }
        for (int64_t i = 0; i < srcVecSize; ++i) {
            if (!opsMap.count(i)) {
                LLVM_DEBUG(llvm::dbgs() << "Missing index " << i << "\n");
                return failure();
            }
        }

        // Enhanced safety checks
        Operation* lastStore = opsMap[srcVecSize-1].first;
        Operation* copyInsertPoint = lastStore->getNextNode();
        bool hasDangerousOp = false;

        // Get all collected operations for quick lookup
        DenseSet<Operation*> collectedOperations;
        for (auto &entry : opsMap) {
            collectedOperations.insert(entry.second.first);  // Store ops
            collectedOperations.insert(entry.second.second); // Load ops
        }

        for (Operation& op : *storeOp->getBlock()) {
            // Stop at insertion point
            if (&op == copyInsertPoint) {
                break;
            }

            // Skip collected operations
            if (collectedOperations.contains(&op)) {
                continue;
            }

            // Any operand uses source/dest vector(Memref)
            for (Value operand : op.getOperands()) {
                if (operand == sourceVec || operand == destVec) {
                    hasDangerousOp = true;
                    break;
                }
            }
            if (hasDangerousOp) {
                break;
            }

            // Any operation using the vectors
            if (auto memOp = dyn_cast<fhe::LoadOp>(&op)) {
                if (memOp.getMemref() == sourceVec || memOp.getMemref() == destVec) {
                    hasDangerousOp = true;
                    break;
                }
            }
            if (auto memOp = dyn_cast<fhe::StoreOp>(&op)) {
                if (memOp.getMemref() == destVec) {
                    hasDangerousOp = true;
                    break;
                }
            }
        }

        if (hasDangerousOp) {
            return failure(); 
        }

        // Create vector copy and clean up
        LLVM_DEBUG(llvm::dbgs() << "Optimizing " << srcVecSize
                                << "-element copy at " << storeOp->getLoc() << "\n");
        rewriter.setInsertionPointAfter(lastStore);
        rewriter.create<fhe::CopyOp>(storeOp.getLoc(), sourceVec, destVec);
        
        // Erase in reverse order to maintain dominance
        SmallVector<Operation*> toErase;
        for (auto &entry : opsMap) {
            toErase.push_back(entry.second.first);  // Store ops
            toErase.push_back(entry.second.second); // Load ops
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
    int target_slot = -1;

    if (mlir::isa<fhe::LoadOp>(op)) {
        auto loadOp = llvm::cast<fhe::LoadOp>(op);
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

        /*
        // Create rotation amount (bring element to position 0)
        auto rotateAmount = ((-target_slot + max_size) % max_size);
        if (NegativeShiftRight) {
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