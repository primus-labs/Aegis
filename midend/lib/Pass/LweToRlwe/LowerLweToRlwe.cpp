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
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Sequence.h"
#include <queue>

#define DEBUG_TYPE "lwr-to-rlwr"

using namespace mlir;
using namespace aegis;
using namespace fhe;


// Transform the batched LWE operator to RLWE operator
template <typename OpType> 
LogicalResult LweBinOpToRlweBinOp(IRRewriter &rewriter, MLIRContext *context, Operation* genericOp,
                                  TypeConverter& typeConverter) {
    auto op = mlir::cast<OpType>(genericOp);
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

    // Deal with binary ops(mul/div/add/sub...)
    if (mlir::isa<fhe::LWEMulOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWEMulOp>(op, destTy, castOps);
        return success();
    } else if (mlir::isa<fhe::LWEMulPlainOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWEMulPlainOp>(op, destTy, castOps);
        return success();
    } else if (mlir::isa<fhe::LWEDivOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWEDivOp>(op, destTy, castOps);
        return success();
    } else if (mlir::isa<fhe::LWEDivPlainOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWEDivPlainOp>(op, destTy, castOps);
        return success();
    } else if (mlir::isa<fhe::LWEAddOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWEAddOp>(op, destTy, castOps);
        return success();
    } else if (mlir::isa<fhe::LWEAddPlainOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWEAddPlainOp>(op, destTy, castOps);
        return success();
    } else if (mlir::isa<fhe::LWESubOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWESubOp>(op, destTy, castOps);
        return success();
    } else if (mlir::isa<fhe::LWESubPlainOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWESubPlainOp>(op, destTy, castOps);
        return success();
    }

    return failure();
}

// Transform the unary LWE operator to RLWE operator
template <typename OpType> 
LogicalResult LweUnaryOpToRlweUnaryOp(IRRewriter &rewriter, MLIRContext *context, Operation* genericOp,
                                      TypeConverter& typeConverter) {
    auto op = mlir::cast<OpType>(genericOp);
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
    if (mlir::isa<fhe::LWENegOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWENegOp>(op, opDestTy, newOpVal);
        return success();
    } else if (mlir::isa<fhe::LWEReciprocalOp>(op)) {
        rewriter.replaceOpWithNewOp<fhe::RLWEReciprocalOp>(op, opDestTy, newOpVal);
        return success();
    }

    return failure();
}

// Convert LWE fhe::xLoadOp to RLWE fhe::xLoadOp
template <typename LoadOpType>
LogicalResult LweLoadToRlweLoad(Operation* op, IRRewriter& rewriter, MLIRContext *context, TypeConverter& typeConverter) {
    auto loadOp = llvm::cast<LoadOpType>(op);
    auto srcTy = loadOp.getMemref().getType();
    auto destTy = typeConverter.convertType(srcTy);
    if (!destTy) {
        return failure();
    }

    Value fheMemrefVal = loadOp.getMemref();
    if (srcTy != destTy) {
        fheMemrefVal = typeConverter.materializeTargetConversion(rewriter, loadOp.getLoc(), destTy, loadOp.getMemref());
        assert(fheMemrefVal && "Type conversion failed");
    }

    auto srcResTy = loadOp.getType();
    auto destResTy = typeConverter.convertType(srcResTy);
    if (!destResTy) {
        return failure();
    }

    using OpCategory = std::conditional_t<std::is_same_v<LoadOpType, fhe::LoadOp>,
                                          std::integral_constant<int, 0>,
                                          std::integral_constant<int, 1>>;
    if constexpr (OpCategory::value == 0) {
        auto indices = loadOp.getIndices();
        rewriter.replaceOpWithNewOp<fhe::LoadOp>(op, destResTy, fheMemrefVal, indices);
    } else {
        auto row = loadOp.getRow();
        rewriter.replaceOpWithNewOp<fhe::VloadOp>(op, destResTy, fheMemrefVal, row);
    }

    return success();
}

// Convert LWE fhe::xStoreOp to RLWE fhe::xStoreOp
template <typename StoreOpType>
LogicalResult LweStoreToRlweStore(Operation* op, IRRewriter& rewriter, MLIRContext *context, TypeConverter& typeConverter) {
    auto storeOp = llvm::cast<StoreOpType>(op);
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
        fheValToStore = typeConverter.materializeTargetConversion(rewriter, storeOp.getLoc(), valueToStoreDestTy, storeOp.getValueToStore());
        assert(fheValToStore);
    }

    Value fheMemrefVal = storeOp.getMemref();
    if (destTy != srcTy) {
        fheMemrefVal = typeConverter.materializeTargetConversion(rewriter, storeOp.getLoc(), destTy, storeOp.getMemref());
        assert(fheMemrefVal);
    }

    using OpCategory = std::conditional_t<std::is_same_v<StoreOpType, fhe::StoreOp>,
                                          std::integral_constant<int, 0>,
                                          std::integral_constant<int, 1>>;
    if constexpr (OpCategory::value == 0) {
        auto indices = storeOp.getIndices();
        rewriter.replaceOpWithNewOp<fhe::StoreOp>(op, fheValToStore, fheMemrefVal, indices);
    } else {
        auto row = storeOp.getRow();
        rewriter.replaceOpWithNewOp<fhe::VstoreOp>(op, fheValToStore, fheMemrefVal, row);
    }
    
    return success();
}

// Transform the LWE memory operator to RLWE memory operator
template <typename OpType> 
LogicalResult LweMemOpToRlweMemOp(IRRewriter &rewriter, MLIRContext *context, Operation* genericOp,
                                  TypeConverter& typeConverter) {
    auto op = mlir::cast<OpType>(genericOp);
    rewriter.setInsertionPoint(op);
    if (mlir::isa<fhe::LoadOp>(op)) {
        return LweLoadToRlweLoad<fhe::LoadOp>(op, rewriter, context, typeConverter);
    } else if (mlir::isa<fhe::VloadOp>(op)) {
        return LweLoadToRlweLoad<fhe::VloadOp>(op, rewriter, context, typeConverter);
    } else if (mlir::isa<fhe::StoreOp>(op)) {
        return LweStoreToRlweStore<fhe::StoreOp>(op, rewriter, context, typeConverter);
    } else if (mlir::isa<fhe::VstoreOp>(op)) {
        return LweStoreToRlweStore<fhe::VstoreOp>(op, rewriter, context, typeConverter);
    } else if (mlir::isa<fhe::CopyOp>(op)) {
        auto copyOp = llvm::cast<fhe::CopyOp>(op);
        auto sourceValTy = typeConverter.convertType(copyOp.getSource().getType());
        if (!sourceValTy) {
            return failure();
        }
        Value sourceVal = copyOp.getSource();
        if (sourceValTy != copyOp.getSource().getType()) {
            sourceVal = typeConverter.materializeTargetConversion(rewriter, copyOp.getLoc(), sourceValTy, copyOp.getSource());
            assert(sourceVal);
        }

        auto srcRetTy = copyOp.getTarget().getType();
        auto destRetTy = typeConverter.convertType(srcRetTy);
        if (!destRetTy) {
            return failure();
        }
        Value targetVal = copyOp.getTarget();
        if (srcRetTy != destRetTy) {
            targetVal = typeConverter.materializeTargetConversion(rewriter, copyOp.getLoc(), destRetTy, copyOp.getTarget());
            assert(targetVal);
        }

        rewriter.replaceOpWithNewOp<fhe::CopyOp>(op, sourceVal, targetVal);
        return success();
    } else if (mlir::isa<fhe::AllocOp>(op)) {
        auto allocOp = llvm::cast<fhe::AllocOp>(op);

        auto destTy = typeConverter.convertType(allocOp.getType());
        if (!destTy) {
            return failure();
        }

        IntegerAttr alignmentAttr;
        auto alignment = allocOp.getAlignment();
        if (alignment.has_value())
            alignmentAttr = rewriter.getI64IntegerAttr(alignment.value());

        rewriter.replaceOpWithNewOp<fhe::AllocOp>(op, destTy, alignmentAttr);
        return success();
    } else if (mlir::isa<fhe::AllocaOp>(op)) {
        auto allocaOp = llvm::cast<fhe::AllocaOp>(op);

        auto destType = typeConverter.convertType(allocaOp.getType());
        if (!destType) {
            return failure();
        }

        rewriter.replaceOpWithNewOp<fhe::AllocaOp>(op, destType);
        return success();
    } else if (mlir::isa<fhe::DeallocOp>(op)) {
        auto deallocOp = llvm::cast<fhe::DeallocOp>(op);

        Value newOperand;
        auto oldOperand = deallocOp.getOperand();
        auto opDestTy = typeConverter.convertType(oldOperand.getType());
        if (!opDestTy) {
            return failure();
        }

        if (oldOperand.getType() != opDestTy) {
            newOperand = typeConverter.materializeTargetConversion(rewriter, deallocOp.getLoc(), opDestTy, oldOperand);
            assert(newOperand);
        } else {
            newOperand = oldOperand;
        }

        rewriter.replaceOpWithNewOp<fhe::DeallocOp>(op, newOperand);
        return success();
    }

    return failure();
}

// Transform the LWE controlflow operator to RLWE controlflow operator
template <typename OpType> 
LogicalResult LweCtrlOpToRlweCtrlOp(IRRewriter &rewriter, MLIRContext *context, Operation* genericOp,
                                    TypeConverter& typeConverter) {
    auto op = mlir::cast<OpType>(genericOp);
    rewriter.setInsertionPoint(op);
    if (mlir::isa<fhe::CmpOp>(op)) {
        auto cmpOp = llvm::cast<fhe::CmpOp>(op);
        auto destTy = typeConverter.convertType(cmpOp.getType());
        if (!destTy) {
            return failure();
        }

        auto lhsOperand = cmpOp.getLhs();
        auto lhsDestTy = typeConverter.convertType(lhsOperand.getType());
        if (!lhsDestTy) {
            return failure();
        }
        Value newLhsOperand;
        if (lhsOperand.getType() != lhsDestTy) {
            newLhsOperand = typeConverter.materializeTargetConversion(rewriter, cmpOp.getLoc(), lhsDestTy, lhsOperand);
            assert(newLhsOperand);
        } else {
            newLhsOperand = lhsOperand;
        }

        auto rhsOperand = cmpOp.getRhs();
        auto rhsDestTy = typeConverter.convertType(rhsOperand.getType());
        if (!rhsDestTy) {
            return failure();
        }
        Value newRhsOperand;
        if (rhsOperand.getType() != rhsDestTy) {
            newRhsOperand = typeConverter.materializeTargetConversion(rewriter, cmpOp.getLoc(), rhsDestTy, rhsOperand);
            assert(newRhsOperand);
        } else {
            newRhsOperand = rhsOperand;
        }

        arith::CmpFPredicate predicate = cmpOp.getPredicate();
        rewriter.replaceOpWithNewOp<fhe::CmpOp>(op, TypeRange(destTy), predicate, newLhsOperand, newRhsOperand);
        return success();
    } else if (mlir::isa<fhe::SelectOp>(op)) {
        auto selectOp = llvm::cast<fhe::SelectOp>(op);

        auto destTy = typeConverter.convertType(selectOp.getType());
        if (!destTy) {
            return failure();
        }

        auto condOperand = selectOp.getCondition();
        auto condDestTy = typeConverter.convertType(condOperand.getType());
        if (!condDestTy) {
            return failure();
        }
        Value newCondOperand;
        if (condOperand.getType() != condDestTy) {
            newCondOperand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), condDestTy, condOperand);
            assert(newCondOperand);
        } else {
            newCondOperand = condOperand;
        }

        auto trueValOperand = selectOp.getTrueValue();
        auto trueDestTy = typeConverter.convertType(trueValOperand.getType());
        if (!trueDestTy) {
            return failure();
        }
        Value newTrueValOperand;
        if (trueValOperand.getType() != trueDestTy) {
            newTrueValOperand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), trueDestTy, trueValOperand);
            assert(newTrueValOperand);
        } else {
            newTrueValOperand = trueValOperand;
        }

        auto falseValOperand = selectOp.getFalseValue();
        auto falseDestTy = typeConverter.convertType(falseValOperand.getType());
        if (!falseDestTy) {
            return failure();
        }
        Value newFalseValOperand;
        if (falseValOperand.getType() != falseDestTy) {
            newFalseValOperand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), falseDestTy, falseValOperand);
            assert(newFalseValOperand);
        } else {
            newFalseValOperand = falseValOperand;
        }

        rewriter.replaceOpWithNewOp<fhe::SelectOp>(op, destTy, newCondOperand, newTrueValOperand, newFalseValOperand);
        return success();
    }

    return failure();
}

// Define operation category tags 
namespace FheOpCategory {
struct Binary;  
struct Unary;   
struct Memory;  
struct Control; 
struct Custom; 
};

// Primary trait template
template <typename OpT>
struct FheOpTraits {
  // Default category is Custom, requiring explicit specialization
  using category = FheOpCategory::Custom;
  
  // Static assert reminds specialization
  static_assert(sizeof(OpT) == 0, 
      "Must specialize FheOpTraits for all handled operation types!");
};

// Specialization binary operations
#define SPECIALIZE_BINARY_OP(OpT) \
template <> \
struct FheOpTraits<OpT> { \
  using category = FheOpCategory::Binary; \
  static constexpr auto converter = &LweBinOpToRlweBinOp<OpT>; \
}

SPECIALIZE_BINARY_OP(fhe::LWESubOp);
SPECIALIZE_BINARY_OP(fhe::LWESubPlainOp);
SPECIALIZE_BINARY_OP(fhe::LWEAddOp);
SPECIALIZE_BINARY_OP(fhe::LWEAddPlainOp); 
SPECIALIZE_BINARY_OP(fhe::LWEMulOp);
SPECIALIZE_BINARY_OP(fhe::LWEMulPlainOp);
SPECIALIZE_BINARY_OP(fhe::LWEDivOp);
SPECIALIZE_BINARY_OP(fhe::LWEDivPlainOp);

// Specialization unary operations
#define SPECIALIZE_UNARY_OP(OpT)           \
template <> \
struct FheOpTraits<OpT> { \
  using category = FheOpCategory::Unary; \
  static constexpr auto converter = &LweUnaryOpToRlweUnaryOp<OpT>; \
};

SPECIALIZE_UNARY_OP(fhe::LWENegOp);
SPECIALIZE_UNARY_OP(fhe::LWEReciprocalOp);

// Specialization memory operations
#define SPECIALIZE_MEMORY_OP(OpT) \
template <> \
struct FheOpTraits<OpT> { \
  using category = FheOpCategory::Memory; \
  static constexpr auto converter = &LweMemOpToRlweMemOp<OpT>; \
};

SPECIALIZE_MEMORY_OP(fhe::LoadOp);
SPECIALIZE_MEMORY_OP(fhe::VloadOp);
SPECIALIZE_MEMORY_OP(fhe::StoreOp);
SPECIALIZE_MEMORY_OP(fhe::VstoreOp);
SPECIALIZE_MEMORY_OP(fhe::CopyOp);
SPECIALIZE_MEMORY_OP(fhe::AllocOp);
SPECIALIZE_MEMORY_OP(fhe::AllocaOp);
SPECIALIZE_MEMORY_OP(fhe::DeallocOp);

// Specialization controlflow operations
#define SPECIALIZE_CONTROL_OP(OpT) \
template <> \
struct FheOpTraits<OpT> { \
  using category = FheOpCategory::Control; \
  static constexpr auto converter = &LweCtrlOpToRlweCtrlOp<OpT>; \
};

SPECIALIZE_CONTROL_OP(fhe::CmpOp);
SPECIALIZE_CONTROL_OP(fhe::SelectOp);

// Dispatch Table Construction
using LweConverterFunc = LogicalResult(*)(IRRewriter&, MLIRContext*, Operation*, TypeConverter&);

// Safe wrapper for getting op name
template <typename OpT>
std::string getFheOpName() {
  return OpT::getOperationName().str();
};

// Build global dispatch table
static const std::unordered_map<std::string, LweConverterFunc>& getDispatchTable() {
  // Thread-safe static initialization
  static const auto dispatchTable = [] {
    std::unordered_map<std::string, LweConverterFunc> table;
    
    // Registered macro: Compile-time trait check
    #define REGISTER_OP(OpT) \
        static_assert( \
            !std::is_same_v<FheOpTraits<OpT>::category, FheOpCategory::Custom>, \
            "FheOpTraits must be specialized for " #OpT); \
        table[getFheOpName<OpT>()] = FheOpTraits<OpT>::converter

    // Register all supported operations
    REGISTER_OP(fhe::LWESubOp);
    REGISTER_OP(fhe::LWESubPlainOp);
    REGISTER_OP(fhe::LWEAddOp);
    REGISTER_OP(fhe::LWEAddPlainOp);
    REGISTER_OP(fhe::LWEMulOp);
    REGISTER_OP(fhe::LWEMulPlainOp);
    REGISTER_OP(fhe::LWEDivOp);
    REGISTER_OP(fhe::LWEDivPlainOp);

    REGISTER_OP(fhe::LWENegOp);
    REGISTER_OP(fhe::LWEReciprocalOp);
    
    REGISTER_OP(fhe::LoadOp);
    REGISTER_OP(fhe::VloadOp);
    REGISTER_OP(fhe::StoreOp);
    REGISTER_OP(fhe::VstoreOp);
    REGISTER_OP(fhe::CopyOp);
    REGISTER_OP(fhe::AllocOp);
    REGISTER_OP(fhe::AllocaOp);
    REGISTER_OP(fhe::DeallocOp);

    REGISTER_OP(fhe::CmpOp);
    REGISTER_OP(fhe::SelectOp);

    #undef REGISTER_OP
    return table;
  }();

  return dispatchTable;
}


void LweToRlwePass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect, mlir::affine::AffineDialect, 
                    func::FuncDialect, mlir::scf::SCFDialect>();
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
            if (mlir::isa<LWECipherType, LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        } else if (mlir::isa<RLWECipherGridType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<LWECipherMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "[LweToRlwePass] Materialization(addTargetMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addArgumentMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<RLWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<LWECipherType, LWECipherVectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        } else if (mlir::isa<RLWECipherGridType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::isa<LWECipherMatrixType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
            }
        }

        llvm::errs() << "[LweToRlwePass] Materialization(addArgumentMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<LWECipherType, LWECipherVectorType>(t)) {
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

        llvm::errs() << "[LweToRlwePass] Materialization(addSourceMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    auto &block = getOperation()->getRegion(0).getBlocks().front();
    IRRewriter rewriter(&getContext());

    // handle all functions
    for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>())) {
        // handle function body stmts
        if (f.walk([&](Operation *op) {
                auto it = getDispatchTable().find(op->getName().getStringRef().str());
                if (it == getDispatchTable().end()) {
                    auto opName = op->getName().getStringRef();
                    if (opName.starts_with("lwe")) {
                        op->emitError() << "Unsupported LWE operation : " << opName.str();
                        return WalkResult::interrupt();
                    }
                    return WalkResult(success());
                }

                // Execute conversion
                if (failed(it->second(rewriter, &getContext(), op, type_converter))) {
                    return WalkResult::interrupt();
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
            if (mlir::isa<LWECipherType, LWECipherVectorType, LWECipherMatrixType>(originalType)) {
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
            if (!(mlir::isa<LWECipherType, LWECipherVectorType, LWECipherMatrixType>(arg.getType()))) {
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
