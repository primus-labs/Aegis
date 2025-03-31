#include "Pass/LweToRlwe/LowerLweToRlwe.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Sequence.h"
#include <queue>

#define DEBUG_TYPE "lwr-to-rlwr"

using namespace mlir;
using namespace aegis;
using namespace fhe;

void LweToRlwePass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, mlir::affine::AffineDialect, 
                    func::FuncDialect, mlir::scf::SCFDialect>();
}

// Transform the batched LWE operator to RLWE operator
template <typename OpType> 
LogicalResult LweBinOpToRlweBinOp(IRRewriter &rewriter, MLIRContext *context, OpType op,
                                    TypeConverter typeConverter) {
    rewriter.setInsertionPoint(op);

    auto destTy = typeConverter.convertType(op.getType());
    if (!destTy) {
        LLVM_DEBUG(llvm::dbgs() << "call convertType fail for the op " << op << "\n");
        return failure();
    }

    llvm::SmallVector<Value> castOps;
    for (Value operand : op.getOperands()) {
        auto operandDestTy = typeConverter.convertType(operand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op " << operand << "\n");
            return failure();
        }
        if (operand.getType() != operandDestTy) {
            auto newOperand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
            assert(newOperand && "Type conversion must not fail");
            castOps.push_back(newOperand);
        } else {
            castOps.push_back(operand);
        }
    }

    // Deal with binary ops(mul/add/sub...)
    if (std::is_same<OpType, fhe::LWEMulOp>()) {
        rewriter.replaceOpWithNewOp<fhe::RLWEMulOp>(op, destTy, castOps);
        return success();
    } else if (std::is_same<OpType, fhe::LWEMulPlainOp>()) {
        rewriter.replaceOpWithNewOp<fhe::RLWEMulPlainOp>(op, destTy, castOps);
        return success();
    } else if (std::is_same<OpType, fhe::LWEAddOp>()) {
        rewriter.replaceOpWithNewOp<fhe::RLWEAddOp>(op, destTy, castOps);
        return success();
    } else if (std::is_same<OpType, fhe::LWEAddPlainOp>()) {
        rewriter.replaceOpWithNewOp<fhe::RLWEAddPlainOp>(op, destTy, castOps);
        return success();
    } else if (std::is_same<OpType, fhe::LWESubOp>()) {
        rewriter.replaceOpWithNewOp<fhe::RLWESubOp>(op, destTy, castOps);
        return success();
    } else if (std::is_same<OpType, fhe::LWESubPlainOp>()) {
        rewriter.replaceOpWithNewOp<fhe::RLWESubPlainOp>(op, destTy, castOps);
        return success();
    }

    return failure();
}

// Transform the unary LWE operator to RLWE operator
template <typename OpType> 
LogicalResult LweUnaryOpToRlweUnaryOp(IRRewriter &rewriter, MLIRContext *context, OpType op,
                                      TypeConverter typeConverter) {
    rewriter.setInsertionPoint(op);

    auto dstType = typeConverter.convertType(op.getType());
    if (!dstType)
        return failure();


    Value opVal = op.getOperand();
    auto opDestTy = typeConverter.convertType(opVal.getType());
    if (!opDestTy) {
        LLVM_DEBUG(llvm::dbgs() << "call convertType fail for value " << opVal << "\n");
        return failure();
    }

    Value newOpVal = opVal;
    if (opVal.getType() != opDestTy) {
        auto newOperand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), opDestTy, opVal);
        assert(newOperand && "Type Conversion must be not fail");
        newOpVal = newOperand;
        LLVM_DEBUG(llvm::dbgs() << "after call materializeTargetConversion, new ops " << newOperand << "\n");
    }

    // Deal with unary ops(neg...)
    if (std::is_same<OpType, fhe::LWENegOp>()) {
        rewriter.replaceOpWithNewOp<fhe::RLWENegOp>(op, opDestTy, newOpVal);
        return success();
    }

    return failure();
}

// Convert fhe Op LWECipher type to RLWECipher type.
template <typename OpType> 
LogicalResult ConvertOpLWETypeToRLWEType(IRRewriter &rewriter, MLIRContext *context, OpType op,
                                         TypeConverter typeConverter) {
    rewriter.setInsertionPoint(op);
    if (std::is_same<OpType, fhe::LoadOp>()) {
        auto loadOp = llvm::cast<fhe::LoadOp>(op);
        auto srcTy = loadOp.getMemref().getType();
        auto destTy = typeConverter.convertType(srcTy);
        if (!destTy) {
            return failure();
        }
        Value fheMemrefVal = loadOp.getMemref();
        if (srcTy != destTy) {
            fheMemrefVal = typeConverter.materializeTargetConversion(rewriter, loadOp.getLoc(), destTy, loadOp.getMemref());
        }

        // Get rlwecipher Plaintext Type
        mlir::Type destUnitTy;
        if (auto CipherTy = mlir::dyn_cast_or_null<fhe::RLWECipherType>(destTy)) {
            destUnitTy = CipherTy.getPlaintextType();
        } else if (auto CipherTy = mlir::dyn_cast_or_null<fhe::RLWECipherGridType>(destTy)) {
            destUnitTy = CipherTy.getPlaintextType();
        }
        auto unitCipherTy = fhe::RLWECipherType::get(context, destUnitTy, 1);
         
        rewriter.replaceOpWithNewOp<fhe::LoadOp>(op, unitCipherTy, fheMemrefVal, loadOp.getIndices());
        return success();
    } else if (std::is_same<OpType, fhe::StoreOp>()) {
        auto storeOp = llvm::cast<fhe::StoreOp>(op);
        auto srcTy = storeOp.getMemref().getType();
        auto destTy = typeConverter.convertType(srcTy);
        if (!destTy) {
            return failure();
        }
        auto valueToStoreDestTy = typeConverter.convertType(storeOp.getValueToStore().getType());
        if (!valueToStoreDestTy) {
            return failure();
        }

        Value fheValToStore = storeOp.getValueToStore();
        if (valueToStoreDestTy != storeOp.getValueToStore().getType()) {
            typeConverter.materializeTargetConversion(rewriter, storeOp.getLoc(), valueToStoreDestTy, storeOp.getValueToStore());
        }

        Value fheMemrefVal = storeOp.getMemref();
        if (destTy != srcTy) {
            fheMemrefVal = typeConverter.materializeTargetConversion(rewriter, storeOp.getLoc(), destTy, storeOp.getMemref());
        }
        
        rewriter.replaceOpWithNewOp<fhe::StoreOp>(op, fheValToStore, fheMemrefVal, storeOp.getIndices());
        return success();
    }

    llvm::outs() << "Catched a unhandle op.\n";
    return success(); 
}


// Tranform pure LWE ciphertexts into RLWE ciphertexts after batching optimizations
void LweToRlwePass::runOnOperation() {
    auto type_converter = TypeConverter();

    type_converter.addConversion([&](Type t) {
        if (mlir::isa<LWECipherType>(t)) {
            auto srcTy = mlir::cast<LWECipherType>(t);
            return std::optional<Type>(RLWECipherType::get(&getContext(), srcTy.getPlaintextType(), 1));
        } else if (mlir::isa<LWECipherVectorType>(t)) {
            auto srcTy = mlir::cast<LWECipherVectorType>(t);
            auto sizes = srcTy.getSize();
            return std::optional<Type>(RLWECipherType::get(&getContext(), srcTy.getPlaintextType(), sizes));
        } else if (mlir::isa<LWECipherMatrixType>(t)) {
            auto srcTy = mlir::cast<LWECipherMatrixType>(t);
            auto row = srcTy.getRow();
            auto col = srcTy.getCol();
            return std::optional<Type>(RLWECipherGridType::get(&getContext(), srcTy.getPlaintextType(), row, col));
        } else {
            return std::optional<Type>(t);
        }
    });

    type_converter.addTargetMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<RLWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<LWECipherType>(srcTy) || mlir::isa<LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        } else if (mlir::isa<RLWECipherGridType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<LWECipherMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "call addTargetMaterialization return null value for type:" << t << ".[LweToRlwe pass]\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<RLWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<LWECipherType>(srcTy) || mlir::isa<LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        } else if (mlir::isa<RLWECipherGridType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<LWECipherMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "call addArgumentMaterialization return null value for type:" << t << ".[LweToRlwe pass]\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<LWECipherType>(t) || mlir::isa<LWECipherVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<RLWECipherType>(srcTy))
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
        } else if ( mlir::isa<LWECipherMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<RLWECipherGridType>(srcTy))
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
        }

        llvm::errs() << "call addSourceMaterialization return null value for type:" << t << ".[LweToRlwe pass]\n";
        return std::optional<Value>(std::nullopt);
    });

    auto &block = getOperation()->getRegion(0).getBlocks().front();
    IRRewriter rewriter(&getContext());

    // handle all functions
    for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        // handle function body stmts
        if (f.walk([&](Operation *op) {
                // binary operator
                if (fhe::LWESubOp subOp = llvm::dyn_cast_or_null<fhe::LWESubOp>(op)) {
                    if (LweBinOpToRlweBinOp<fhe::LWESubOp>(rewriter, &getContext(), subOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (fhe::LWESubPlainOp subPlainOp = llvm::dyn_cast_or_null<fhe::LWESubPlainOp>(op)) {
                    if (LweBinOpToRlweBinOp<fhe::LWESubPlainOp>(rewriter, &getContext(), subPlainOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (fhe::LWEAddOp addOp = llvm::dyn_cast_or_null<fhe::LWEAddOp>(op)) {
                    if (LweBinOpToRlweBinOp<fhe::LWEAddOp>(rewriter, &getContext(), addOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (fhe::LWEAddPlainOp addPlainOp = llvm::dyn_cast_or_null<fhe::LWEAddPlainOp>(op)) {
                    if (LweBinOpToRlweBinOp<fhe::LWEAddPlainOp>(rewriter, &getContext(), addPlainOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (fhe::LWEMulOp mulOp = llvm::dyn_cast_or_null<fhe::LWEMulOp>(op)) {
                    if (LweBinOpToRlweBinOp<fhe::LWEMulOp>(rewriter, &getContext(), mulOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (fhe::LWEMulPlainOp mulPlainOp = llvm::dyn_cast_or_null<fhe::LWEMulPlainOp>(op)) {
                    if (LweBinOpToRlweBinOp<fhe::LWEMulPlainOp>(rewriter, &getContext(), mulPlainOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                // unary operator
                } else if (fhe::LWENegOp negOp = llvm::dyn_cast_or_null<fhe::LWENegOp>(op)) {
                    if (LweUnaryOpToRlweUnaryOp<fhe::LWENegOp>(rewriter, &getContext(), negOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                // load/store operator
                } else if (fhe::LoadOp loadOp = llvm::dyn_cast_or_null<fhe::LoadOp>(op)) {
                    if (ConvertOpLWETypeToRLWEType<fhe::LoadOp>(rewriter, &getContext(), loadOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                } else if (fhe::StoreOp storeOp = llvm::dyn_cast_or_null<fhe::StoreOp>(op)){
                    if (ConvertOpLWETypeToRLWEType<fhe::StoreOp>(rewriter, &getContext(), storeOp, type_converter).failed()) {
                        return WalkResult::interrupt();
                    }
                }
                return WalkResult(success());
             }).wasInterrupted()) {
            signalPassFailure();
        }

        // handle function prototype
        func::FuncOp op = f;

        // Generate the new signature of the function.
        SmallVector<Type> newResTypes;
        if (failed(type_converter.convertTypes(op.getFunctionType().getResults(), newResTypes))) {
            signalPassFailure();
        }

        // Iterate through all parameters and process them one by one.
        TypeConverter::SignatureConversion signatureConversion(op.getFunctionType().getNumInputs());
        for (auto [index, arg] : llvm::enumerate(op.getRegion().getArguments())) {
            Type originalType = op.getFunctionType().getInput(index);
            if (mlir::isa<fhe::LWECipherType>(originalType) || 
                mlir::isa<fhe::LWECipherVectorType>(originalType) ||
                mlir::isa<fhe::LWECipherMatrixType>(originalType)) {
                SmallVector<Type> destTypes;
                if (failed(type_converter.convertType(originalType, destTypes))) {
                    signalPassFailure();
                }
                signatureConversion.addInputs(index, destTypes);
            } else {
                signatureConversion.addInputs(index, {originalType});
            }
        }

        auto newFuncTy = FunctionType::get(&getContext(), signatureConversion.getConvertedTypes(), newResTypes);
        rewriter.startOpModification(op);
        op.setType(newFuncTy);
        for (BlockArgument arg : op.getRegion().getArguments()) {
            if (!(mlir::isa<fhe::LWECipherType>(arg.getType()) || 
                  mlir::isa<fhe::LWECipherVectorType>(arg.getType()) ||
                  mlir::isa<fhe::LWECipherMatrixType>(arg.getType()))) {
                continue;
            }

            auto oldType = arg.getType();
            auto newType = type_converter.convertType(oldType);
            assert(newType);
            arg.setType(newType);
            if (newType != oldType) {
                rewriter.setInsertionPointToStart(&op.getBody().getBlocks().front());
                auto cast_op = type_converter.materializeSourceConversion(rewriter, arg.getLoc(), oldType, arg);
                arg.replaceAllUsesExcept(cast_op, cast_op.getDefiningOp());
            }
        }

        // handle function return stmt
        for (auto &block : op.getBody()) {
            for (auto retOp : llvm::make_early_inc_range(block.getOps<func::ReturnOp>())) {
                SmallVector<Value, 4> newOperands;
                for (auto operand : retOp.getOperands()) {
                    auto oldType = operand.getType();
                    auto newType = type_converter.convertType(oldType);
                    if (newType != oldType) {
                        rewriter.setInsertionPoint(retOp);
                        auto convertedOperand =
                            type_converter.materializeTargetConversion(rewriter, retOp.getLoc(), newType, operand);
                        if (!convertedOperand) {
                            emitError(retOp.getLoc(), "Failed to convert return operand type");
                            signalPassFailure();
                        }
                        newOperands.push_back(convertedOperand);
                    } else {
                        newOperands.push_back(operand);
                    }
                }
                rewriter.eraseOp(retOp);
                rewriter.setInsertionPointToEnd(&block);
                rewriter.create<func::ReturnOp>(retOp.getLoc(), newOperands);
            }
        }

        rewriter.finalizeOpModification(op);
    }
}