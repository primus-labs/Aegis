#include <string>

#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Conversion/MemRefToEmitC/MemRefToEmitC.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/IR/BuiltinOps.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/FheToEmitc/LowerFheToEmitc.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "fhe-to-emitc"

using namespace mlir;
using namespace aegis;
using namespace fhe;


// Convert FHE arith unary op(neg/abs/...) operations into emitc::CallOp.
template <typename OpType> class FheArithUnaryPattern final : public OpConversionPattern<OpType> {
  protected:
    using OpConversionPattern<OpType>::typeConverter;

  public:
    using OpConversionPattern<OpType>::OpConversionPattern;

    LogicalResult matchAndRewrite(OpType op, typename OpType::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" 
                                    << op.getType() << ".\n");
            return failure();
        }

        // Materialize the operand where necessary
        Value newOperand;
        auto operand = op.getOperand();
        auto operandDestTy = typeConverter->convertType(operand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << operand
                                    << ", the op type:" << operand.getType() << ".\n");
            return failure();
        }

        if (operand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
            assert(newOperand);
        } else {
            newOperand = operand;
        }

        // Build a series of calls to our custom function.
        std::string opName;
        if (std::is_same<OpType, fhe::LWENegOp>() || std::is_same<OpType, fhe::RLWENegOp>()) {
            opName = "Neg";
        } else {
            LLVM_DEBUG(llvm::dbgs() << "Unkown the Op:" << OpType::getOperationName() << "not handle.\n");
            return failure();
        }

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), opName, ArrayAttr(), ArrayAttr(),
                                                         newOperand);
        return success();
    }
};

// Convert FHE arith binary op(add/sub/mul/...) operations into emitc::CallOp.
template <typename OpType>
class FheArithBinaryPattern final : public OpConversionPattern<OpType> {
protected:
    using OpConversionPattern<OpType>::typeConverter;

public:
    using OpConversionPattern<OpType>::OpConversionPattern;

    LogicalResult matchAndRewrite(OpType op, typename OpType::Adaptor adaptor, 
                                  ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op 
                                    << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        // Materialize the operands where necessary
        llvm::SmallVector<Value> materialized_ops;
        for (auto operand : op.getOperands()) {
            auto operandDestTy = typeConverter->convertType(operand.getType());
            if (!operandDestTy) {
                LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << operand 
                                        << ", the op type:" << operand.getType() << ".\n");
                return failure();
            }

            if (operand.getType() != operandDestTy) {
                auto newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
                assert(newOperand);
                materialized_ops.push_back(newOperand);
            }
            else {
                materialized_ops.push_back(operand);
            }
        }

        // Build a series of calls to our custom function.
        std::string opName;
        if (std::is_same<OpType, LWEAddOp>() || 
            std::is_same<OpType, RLWEAddOp>()) {
            opName = "Add";
        }
        else if (std::is_same<OpType, LWEAddPlainOp>() ||
                 std::is_same<OpType, RLWEAddPlainOp>()) {
            opName = "AddPlain";
        }
        else if (std::is_same<OpType, LWESubOp>() ||
                 std::is_same<OpType, RLWESubOp>()) {
            opName = "Sub";
        }
        else if (std::is_same<OpType, LWESubPlainOp>() ||
                 std::is_same<OpType, RLWESubPlainOp>()) {
            opName = "SubPlain";
        }
        else if (std::is_same<OpType, LWEMulOp>() ||
                 std::is_same<OpType, RLWEMulOp>()) {
            opName = "Mul";
        }
        else if (std::is_same<OpType, LWEMulPlainOp>() ||
                 std::is_same<OpType, RLWEMulPlainOp>()) {
            opName = "MulPlain";
        }
        else {
            llvm::errs() << "[FhetoEmitcPass] ERROR: Unhandled operation detected\n"
                         << "   Operation: " << op << "\n"
                         << "   Location:  " << op.getLoc() << "\n";
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), opName, 
                                    ArrayAttr(), ArrayAttr(), materialized_ops);
        return success();
    }
};


// Transform fhe::RotateOp to emitc::CallOpaqueOp.
class FheRotatePattern final : public OpConversionPattern<fhe::RotateOp> {
  public:
    using OpConversionPattern<fhe::RotateOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::RotateOp op, typename fhe::RotateOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        // get rotate index and save to metadata
        auto index = op.getI();
        ModuleOp moduleOp = op->getParentOfType<ModuleOp>();
        mlir::aegis::addGaloisIndex(moduleOp, index);

        // convert to emitc type
        auto resTy = getTypeConverter()->convertType(op.getType());
        if (!resTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" 
                                    << op.getType() << ".\n");
            return failure();
        }

        Value operand = op.getCipher();
        auto operandDestTy = typeConverter->convertType(operand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << operand
                                    << ", the op type:" << operand.getType() << ".\n");
            return failure();
        }

        Value newOperand;
        if (operand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
            assert(newOperand);
        } else {
            newOperand = operand;
        }

        auto rotIdxAttr = ArrayAttr::get(
            getContext(), {IntegerAttr::get(IndexType::get(getContext()), 0), rewriter.getSI32IntegerAttr(op.getI())});

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, resTy, "Rotate", rotIdxAttr, ArrayAttr(), newOperand);

        return success();
    }
};

// Transform fhe::BootstrapOp to emitc::CallOpaqueOp.
class FheBootstrapPattern final : public OpConversionPattern<fhe::BootstrapOp> {
  public:
    using OpConversionPattern<fhe::BootstrapOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::BootstrapOp op, typename fhe::BootstrapOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op 
                                    << ", the op type:"  << op.getType() << ".\n");
            return failure();
        }

        Value newOperand;
        auto input = op.getInput();
        auto inputDestTy = typeConverter->convertType(input.getType());
        if (!inputDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << input 
                                    << ", the op type:" << input.getType() << ".\n");
            return failure();
        }

        if (input.getType() != inputDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), inputDestTy, input);
            assert(newOperand);
        } else {
            newOperand = input;
        }

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "Bootstrap", ArrayAttr(), ArrayAttr(),
                                                         newOperand);
        return success();
    }
};

// Transfor fhe::CmpOp to emitc::CallOpaqueOp
class FheCmpPattern final : public OpConversionPattern<fhe::CmpOp> {
  public:
    using OpConversionPattern<fhe::CmpOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::CmpOp op, typename fhe::CmpOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op 
                                    << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        auto lhsOperand = op.getLhs();
        auto lhsDestTy = typeConverter->convertType(lhsOperand.getType());
        if (!lhsDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << lhsOperand
                                    << ", the op type:" << lhsOperand.getType() << ".\n");
            return failure();
        }
        Value newLhsOperand;
        if (lhsOperand.getType() != lhsDestTy) {
            newLhsOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), lhsDestTy, lhsOperand);
            assert(newLhsOperand);
        } else {
            newLhsOperand = lhsOperand;
        }

        auto rhsOperand = op.getRhs();
        auto rhsDestTy = typeConverter->convertType(rhsOperand.getType());
        if (!rhsDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << rhsOperand
                                    << ", the op type:" << rhsOperand.getType() << ".\n");
            return failure();
        }
        Value newRhsOperand;
        if (rhsOperand.getType() != rhsDestTy) {
            newRhsOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), rhsDestTy, rhsOperand);
            assert(newRhsOperand);
        } else {
            newRhsOperand = rhsOperand;
        }

        std::string suffix;
        arith::CmpFPredicate predicate = op.getPredicate();
        switch (predicate) {
            case arith::CmpFPredicate::UEQ:
            case arith::CmpFPredicate::OEQ:
                suffix = "eq";
                break;
            case arith::CmpFPredicate::UGT:
            case arith::CmpFPredicate::OGT:
                suffix = "gt";
                break;
            case arith::CmpFPredicate::UGE:
            case arith::CmpFPredicate::OGE:
                suffix = "ge";
                break;
            case arith::CmpFPredicate::ULT:
            case arith::CmpFPredicate::OLT:
                suffix = "lt";
                break;
            case arith::CmpFPredicate::ULE:
            case arith::CmpFPredicate::OLE:
                suffix = "le";
                break;
            case arith::CmpFPredicate::UNE:
            case arith::CmpFPredicate::ONE:
                suffix = "ue";
                break;
            default:
                llvm_unreachable("Unexpected predicate!");
        }

        std::string funcName = "Cmp_" + suffix;
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), funcName, ArrayAttr(), ArrayAttr(),
                                                         ValueRange({newLhsOperand, newRhsOperand}));
        return success();
    }
};

// Transfor fhe::SelectOp to emitc::CallOpaqueOp
class FheSelectPattern final : public OpConversionPattern<fhe::SelectOp> {
  public:
    using OpConversionPattern<fhe::SelectOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::SelectOp op, typename fhe::SelectOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getType()
                                    << ".\n");
            return failure();
        }

        auto condOperand = op.getCondition();
        auto condDestTy = typeConverter->convertType(condOperand.getType());
        if (!condDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << condOperand
                                    << ", the op type:" << condOperand.getType() << ".\n");
            return failure();
        }
        Value newCondOperand;
        if (condOperand.getType() != condDestTy) {
            newCondOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), condDestTy, condOperand);
            assert(newCondOperand);
        } else {
            newCondOperand = condOperand;
        }

        auto trueValOperand = op.getTrueValue();
        auto trueDestTy = typeConverter->convertType(trueValOperand.getType());
        if (!trueDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << trueValOperand
                                    << ", the op type:" << trueValOperand.getType() << ".\n");
            return failure();
        }
        Value newTrueValOperand;
        if (trueValOperand.getType() != trueDestTy) {
            newTrueValOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), trueDestTy, trueValOperand);
            assert(newTrueValOperand);
        } else {
            newTrueValOperand = trueValOperand;
        }

        auto falseValOperand = op.getFalseValue();
        auto falseDestTy = typeConverter->convertType(falseValOperand.getType());
        if (!falseDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << falseValOperand
                                    << ", the op type:" << falseValOperand.getType() << ".\n");
            return failure();
        }
        Value newFalseValOperand;
        if (falseValOperand.getType() != falseDestTy) {
            newFalseValOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), falseDestTy, falseValOperand);
            assert(newFalseValOperand);
        } else {
            newFalseValOperand = falseValOperand;
        }

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "Select", ArrayAttr(), ArrayAttr(),
                                                         ValueRange({newCondOperand, newTrueValOperand, newFalseValOperand}));
        return success();
    }
};

// Transform fhe::CallOp to emitc::CallOpaqueOp.
class FheCallPattern final : public OpConversionPattern<func::CallOp> {
  public:
    using OpConversionPattern<func::CallOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::CallOp op, typename func::CallOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        auto resTy = getTypeConverter()->convertType(op.getResult(0).getType());
        if (!resTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op
                                    << ", the op type:" << op.getResult(0).getType() << ".\n");
            return failure();
        }

        llvm::SmallVector<Value> materialized_ops;
        for (Value operand : adaptor.getOperands()) {
            auto operandDestTy = typeConverter->convertType(operand.getType());
            if (!operandDestTy) {
                return failure();
            }

            if (operand.getType() != operandDestTy) {
                auto newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
                assert(newOperand);
                materialized_ops.push_back(newOperand);
            } else {
                materialized_ops.push_back(operand);
            }
        }

        auto funcName = op.getCallee();
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(resTy), funcName, ArrayAttr(), ArrayAttr(),
                                                         materialized_ops);

        return success();
    }
};


class FheFuncPattern final : public OpConversionPattern<func::FuncOp> {
  public:
    using OpConversionPattern<func::FuncOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::FuncOp op, typename func::FuncOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        TypeConverter::SignatureConversion signatureConv(op.getFunctionType().getNumInputs());
        SmallVector<Type> newResTypes;
        if (failed(typeConverter->convertTypes(op.getFunctionType().getResults(), newResTypes))) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op
                                    << ", the op type:" << op.getFunctionType().getResults() << ".\n");
            return failure();
        }
        if (typeConverter->convertSignatureArgs(op.getFunctionType().getInputs(), signatureConv).failed()) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op
                                    << ", the op type:" << op.getFunctionType().getInputs() << ".\n");
            return failure();
        }
        auto newFuncTy = FunctionType::get(getContext(), signatureConv.getConvertedTypes(), newResTypes);

        rewriter.startOpModification(op);
        op.setType(newFuncTy);
        for (BlockArgument arg : op.getRegion().getArguments()) {
            auto srcTy = arg.getType();
            auto destTy = typeConverter->convertType(srcTy);
            arg.setType(destTy);
            if (destTy != srcTy) {
                rewriter.setInsertionPointToStart(&op.getBody().getBlocks().front());
                auto castOp = typeConverter->materializeSourceConversion(rewriter, arg.getLoc(), srcTy, arg);
                arg.replaceAllUsesExcept(castOp, castOp.getDefiningOp());
            }
        }
        rewriter.finalizeOpModification(op);

        return success();
    }
};

class FheRetPattern final : public OpConversionPattern<func::ReturnOp> {
  public:
    using OpConversionPattern<func::ReturnOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::ReturnOp op, typename func::ReturnOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        if (op->getNumOperands() != 1) {
            emitError(op->getLoc(), "Currently, only single value returns are supported.");
            return failure();
        }
        auto destTy = this->getTypeConverter()->convertType(op->getOperandTypes().front());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op
                                    << ", the op type:" << op->getOperandTypes().front() << ".\n");
            return failure();
        }

        if (auto _ = mlir::dyn_cast_or_null<emitc::OpaqueType>(destTy)) {
            rewriter.setInsertionPoint(op);
            auto castOp = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), destTy, op.getOperands());
            assert(castOp);
            rewriter.replaceOpWithNewOp<func::ReturnOp>(op, castOp);
        }

        return success();
    }
};

// Transform memref::GetGlobalOp to emitc::GetGlobalOp
class MemrefGetGlobalPattern final : public OpConversionPattern<memref::GetGlobalOp> {
  protected:
    using OpConversionPattern<memref::GetGlobalOp>::typeConverter;

  public:
    using OpConversionPattern<memref::GetGlobalOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::GetGlobalOp op, typename memref::GetGlobalOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        // auto resTy = getTypeConverter()->convertType(op.getType());
        // if (!resTy) {
        //     return rewriter.notifyMatchFailure(op.getLoc(), "cannot convert result type");
        // }

        emitc::ArrayType resTy;
        if (!op.getType().getShape().size()) {
            resTy = emitc::ArrayType::get(getContext(), 1, op.getType().getElementType());
        }
        else {
            resTy = emitc::ArrayType::get(getContext(), op.getType().getShape(), op.getType().getElementType());
        }
        rewriter.replaceOpWithNewOp<emitc::GetGlobalOp>(op, resTy, adaptor.getNameAttr());

        return success();
    }
};

// Transform memref::GlobalOp to emitc::GlobalOp
class MemrefGlobalPattern final : public OpConversionPattern<memref::GlobalOp> {
  protected:
    using OpConversionPattern<memref::GlobalOp>::typeConverter;

  public:
    using OpConversionPattern<memref::GlobalOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::GlobalOp op, typename memref::GlobalOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        if (!op.getType().hasStaticShape()) {
            return rewriter.notifyMatchFailure(op.getLoc(), "cannot transform global with dynamic shape");
        }

        if (op.getAlignment().value_or(1) > 1) {
            return rewriter.notifyMatchFailure(op.getLoc(),
                                               "global variable with alignment requirement is currently not supported");
        }

        // auto resTy = getTypeConverter()->convertType(op.getType());
        // if (!resTy) {
        //     return rewriter.notifyMatchFailure(op.getLoc(), "cannot convert global op result type");
        // }
        emitc::ArrayType resTy;
        if (!op.getType().getShape().size()) {
            resTy = emitc::ArrayType::get(getContext(), 1, op.getType().getElementType());
        }
        else {
            resTy = emitc::ArrayType::get(getContext(), op.getType().getShape(), op.getType().getElementType());
        }

        SymbolTable::Visibility visibility = SymbolTable::getSymbolVisibility(op);
        if (visibility != SymbolTable::Visibility::Public && visibility != SymbolTable::Visibility::Private) {
            return rewriter.notifyMatchFailure(op.getLoc(),
                                               "only public and private visibility is currently supported");
        }

        // We are explicit in specifing the linkage because the default linkage
        // for constants is different in C and C++.
        bool staticSpecifier = visibility == SymbolTable::Visibility::Private;
        bool externSpecifier = !staticSpecifier;

        if (!op.getType().getShape().size()) {
            // Get the original initial value (scalar tensor<f64>)
            Attribute oldInitialValue = adaptor.getInitialValueAttr();

            // Create a tensor type with shape [1] = tensor<1xf64>
            if (auto denseAttr = mlir::dyn_cast<DenseFPElementsAttr>(oldInitialValue)) {
                auto tensorType = mlir::cast<TensorType>(denseAttr.getType());
                auto elementType = tensorType.getElementType();
                auto tensor1DType = RankedTensorType::get({1}, elementType);

                // Wrap scalar into array
                SmallVector<Attribute> values;
                double scalarValue = denseAttr.getSplatValue<APFloat>().convertToFloat();
                values.push_back(FloatAttr::get(elementType, scalarValue));

                // Replace the emit GlobalOp operation
                auto d1InitialValue = DenseElementsAttr::get(tensor1DType, values);
                rewriter.replaceOpWithNewOp<emitc::GlobalOp>(op, adaptor.getSymName(), resTy, d1InitialValue, externSpecifier,
                                                             staticSpecifier, adaptor.getConstant());   
            } else {
                return failure();
            }      
        } else {
            Attribute initialValue = adaptor.getInitialValueAttr();
            if (isa_and_present<UnitAttr>(initialValue)) {
                initialValue = {};
            }

            rewriter.replaceOpWithNewOp<emitc::GlobalOp>(op, adaptor.getSymName(), resTy, initialValue, externSpecifier,
                                                         staticSpecifier, adaptor.getConstant());
        }
        return success();
    }
};

// Transform arith::ConstantOp to emitc::ConstantOp
class FheConstantPattern final : public OpConversionPattern<arith::ConstantOp> {
  protected:
    using OpConversionPattern<arith::ConstantOp>::typeConverter;

  public:
    using OpConversionPattern<arith::ConstantOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(arith::ConstantOp op, typename arith::ConstantOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op 
                                    << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        // Get arith::ConstOp interger value.
        bool isAryType = false;
        std::string strVal;
        auto valueAttr = op.getValue();
        if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(valueAttr)) {
            auto nVal = intAttr.getInt();
            strVal = std::to_string(nVal);
        } else if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(valueAttr)) {
            auto dVal = floatAttr.getValueAsDouble();
            strVal = std::to_string(dVal);
        }
        if (auto denseAttr = mlir::dyn_cast<mlir::DenseElementsAttr>(valueAttr)) {
            isAryType = true;

            auto processValues = [&strVal](auto vals) {
                for (size_t i = 0; i < vals.size(); ++i) {
                    strVal += std::to_string(vals[i]);
                    if (i != vals.size() - 1) {
                        strVal += ",";
                    }
                }
            };

            Type elementType = denseAttr.getElementType();
            if (elementType.isF64()) {
                processValues(denseAttr.getValues<double>());
            } else if (elementType.isF32()) {
                processValues(denseAttr.getValues<float>());
            } else if (elementType.isInteger(32)) {
                processValues(denseAttr.getValues<int32_t>());
            } else if (elementType.isInteger(64)) {
                processValues(denseAttr.getValues<int64_t>());
            } else {
                llvm_unreachable("Unsupported element type.(FheConstantPattern::matchAndRewrite)\n");
            }
        }

        // Combine emitc::OpaqueAttr using the value.
        emitc::OpaqueAttr emitcAttrVal;
        if (isAryType) {
            emitcAttrVal = emitc::OpaqueAttr::get(getContext(), ("MakeMultPlain(" + strVal + ")"));
        } else {
            emitcAttrVal = emitc::OpaqueAttr::get(getContext(), ("MakePlain(" + strVal + ")"));
        }
        rewriter.replaceOpWithNewOp<emitc::ConstantOp>(op, TypeRange(destTy), emitcAttrVal);

        return success();
    }
};

// Transform memref::LoadOp to emitc::LoadOp.
// mark: current mlir version not support emitc::LoadOp.
// class MemrefLoadPattern final : public OpConversionPattern<memref::LoadOp> {
// public:
//     using OpConversionPattern<memref::LoadOp>::OpConversionPattern;

//     LogicalResult matchAndRewrite(memref::LoadOp op, typename memref::LoadOp::Adaptor adaptor,
//                                   ConversionPatternRewriter &rewriter) const override
//     {
//         auto resultTy = getTypeConverter()->convertType(op.getType());
//         if (!resultTy) {
//             return rewriter.notifyMatchFailure(op.getLoc(), "cannot convert type");
//         }

//         auto arrayValue = mlir::dyn_cast<TypedValue<emitc::ArrayType>>(adaptor.getMemref());
//         if (!arrayValue) {
//             return rewriter.notifyMatchFailure(op.getLoc(), "expected array type");
//         }

//         auto subscript = rewriter.create<emitc::SubscriptOp>(
//             op.getLoc(), arrayValue, adaptor.getIndices());

//         rewriter.replaceOpWithNewOp<emitc::LoadOp>(op, resultTy, subscript);

//         return success();
//     }
// };


// Transform memref::LoadOp to emitc::LoadOp.
class NativeMemrefLoadPattern final : public OpConversionPattern<memref::LoadOp> {
  protected:
    using OpConversionPattern<memref::LoadOp>::typeConverter;

  public:
    using OpConversionPattern<memref::LoadOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::LoadOp op, typename memref::LoadOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        auto resTy = getTypeConverter()->convertType(op.getType());
        if (!resTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op 
                                    << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        Value memOperand = op.getMemref();
        auto operandDestTy = typeConverter->convertType(memOperand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << memOperand
                                    << ", the op type:" << memOperand.getType() << ".\n");
            return failure();
        }

        Value newOperand;
        if (memOperand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, memOperand);
            assert(newOperand);
        } else {
            newOperand = memOperand;
        }

        SmallVector<Value, 4> indices(op.getIndices().begin(), op.getIndices().end());
        SmallVector<Value, 8> operands;
        operands.push_back(newOperand);
        operands.append(indices.begin(), indices.end());
        if (indices.size() > 0) {
            rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, resTy, "LoadPlainWithIndex", operands);
        } else {
            rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, resTy, "LoadPlainWithoutIndex", operands);
        }

        return success();
    }
};


template <typename OpType> 
class FheLoadLikePattern : public OpConversionPattern<OpType> {
  public:
    using OpConversionPattern<OpType>::OpConversionPattern;

    LogicalResult matchAndRewrite(OpType op, typename OpType::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        // Convert result type
        Type destTy = this->getTypeConverter()->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "Type conversion failed for: " << op << "\n");
            return failure();
        }

        // Process memref operand
        Value memOperand = op.getMemref();
        Value newOperand;
        if (failed(processMemRef(memOperand, op.getLoc(), rewriter, newOperand))) {
            return failure();
        }

        // Collect operands
        SmallVector<Value> operands{newOperand};
        SmallVector<Value> additional = getAdditionalOperands(op);
        operands.append(additional.begin(), additional.end());

        // Create replacement op
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, destTy, getFunctionName(), operands);
        return success();
    }

  protected:
    virtual StringRef getFunctionName() const = 0;
    virtual SmallVector<Value> getAdditionalOperands(OpType op) const = 0;

  private:
    LogicalResult processMemRef(Value memOperand, Location loc, ConversionPatternRewriter &rewriter,
                                Value &newOperand) const {
        auto *typeConverter = this->getTypeConverter();
        Type operandDestTy = typeConverter->convertType(memOperand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "Operand type conversion failed for: " << memOperand << "\n");
            return failure();
        }

        if (memOperand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, loc, operandDestTy, memOperand);
            assert(newOperand && "Materialization failed");

            // Handle nested cast operations
            if (auto castOp = mlir::dyn_cast<fhe::CastOp>(memOperand.getDefiningOp())) {
                if (auto innerCast = mlir::dyn_cast_or_null<fhe::CastOp>(castOp.getOperand().getDefiningOp())) {
                    newOperand = innerCast.getOperand();
                }
            }
        } else {
            newOperand = memOperand;
        }
        return success();
    }
};

// Transform fhe::LoadOp to emitc::CallOpaqueOp.
class FheLoadPattern final : public FheLoadLikePattern<fhe::LoadOp> {
  public:
    using FheLoadLikePattern::FheLoadLikePattern;

  protected:
    StringRef getFunctionName() const override { return "Load"; }

    SmallVector<Value> getAdditionalOperands(fhe::LoadOp op) const override {
        return {op.getIndices().begin(), op.getIndices().end()};
    }
};

// Transform fhe::VloadOp to emitc::CallOpaqueOp.
class FheVloadPattern final : public FheLoadLikePattern<fhe::VloadOp> {
  public:
    using FheLoadLikePattern::FheLoadLikePattern;
    
  protected:
    StringRef getFunctionName() const override { return "Vload"; }

    SmallVector<Value> getAdditionalOperands(fhe::VloadOp op) const override { return {op.getRow()}; }
};


template <typename OpType> 
class FheStoreLikePattern : public OpConversionPattern<OpType> {
  public:
    using OpConversionPattern<OpType>::OpConversionPattern;

    LogicalResult matchAndRewrite(OpType op, typename OpType::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        // Convert memref type
        Type memrefTy = this->typeConverter->convertType(op.getMemref().getType());
        if (!memrefTy) {
            LLVM_DEBUG(llvm::dbgs() << "Memref type conversion failed for: " << op.getMemref() << "\n");
            return failure();
        }

        // Convert value type
        Type valueTy = this->typeConverter->convertType(op.getValueToStore().getType());
        if (!valueTy) {
            LLVM_DEBUG(llvm::dbgs() << "Value type conversion failed for: " << op.getValueToStore() << "\n");
            return failure();
        }

        // Materialize operands
        Value newMemref = materializeConversion(op.getMemref(), memrefTy, rewriter);
        Value newValue = materializeConversion(op.getValueToStore(), valueTy, rewriter);
        if (!newMemref || !newValue) {
            return failure();
        }

        // Collect operands
        SmallVector<Value> operands{newValue, newMemref};
        SmallVector<Value> additional = getAdditionalOperands(op);
        operands.append(additional.begin(), additional.end());

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange{}, getFunctionName(), operands);
        return success();
    }

  protected:
    virtual StringRef getFunctionName() const = 0;
    virtual SmallVector<Value> getAdditionalOperands(OpType op) const = 0;

  private:
    Value materializeConversion(Value origValue, Type destTy, ConversionPatternRewriter &rewriter) const {
        if (origValue.getType() == destTy)
            return origValue;

        return this->typeConverter->materializeTargetConversion(rewriter, origValue.getLoc(), destTy, origValue);
    }
};

// Transform fhe::Store to emitc::CallOpaqueOp.
class FheStorePattern final : public FheStoreLikePattern<fhe::StoreOp> {
  public:
    using FheStoreLikePattern::FheStoreLikePattern;

  protected:
    StringRef getFunctionName() const override { return "Store"; }

    SmallVector<Value> getAdditionalOperands(fhe::StoreOp op) const override {
        return {op.getIndices().begin(), op.getIndices().end()};
    }
};

// Transform fhe::Vstore to emitc::CallOpaqueOp.
class FheVstorePattern final : public FheStoreLikePattern<fhe::VstoreOp> {
  public:
    using FheStoreLikePattern::FheStoreLikePattern;

  protected:
    StringRef getFunctionName() const override { return "Vstore"; }

    SmallVector<Value> getAdditionalOperands(fhe::VstoreOp op) const override {
        return {op.getRow()}; 
    }
};

// Transform fhe::CopyOp to emitc::CallOpaqueOp.
class FheCopyPattern final : public OpConversionPattern<fhe::CopyOp> {
  public:
    using OpConversionPattern<fhe::CopyOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::CopyOp op, typename fhe::CopyOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        rewriter.setInsertionPoint(op);

        auto newSrcTy = typeConverter->convertType(op.getSource().getType());
        if (!newSrcTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op.getSource()
                                    << ", the op type:" << op.getSource().getType() << ".\n");
            return failure();
        }

        auto newDestTy = typeConverter->convertType(op.getTarget().getType());
        if (!newDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op.getTarget()
                                    << ", the op type:" << op.getTarget().getType() << ".\n");
            return failure();
        }

        auto newSrcVal = typeConverter->materializeTargetConversion(rewriter, op.getSource().getLoc(), newSrcTy, op.getSource());
        auto newDestVal = typeConverter->materializeTargetConversion(rewriter, op.getTarget().getLoc(), newDestTy, op.getTarget());
        assert(newSrcVal && newDestVal);
        llvm::SmallVector<Value, 8> operands;
        operands.push_back(newSrcVal);
        operands.push_back(newDestVal);
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange{}, "Copy", operands);

        return success();
    }
};

// Transform fhe::AllocaOp to emitc::CallOpaqueOp.
class FheAllocaPattern final : public OpConversionPattern<fhe::AllocaOp> {
  public:
    using OpConversionPattern<fhe::AllocaOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::AllocaOp op, typename fhe::AllocaOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op 
                                    << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, destTy, "Alloca", ValueRange{});
        return success();
    }
};

// Transform fhe::AllocOp to emitc::CallOpaqueOp.
class FheAllocPattern final : public OpConversionPattern<fhe::AllocOp> {
  public:
    using OpConversionPattern<fhe::AllocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::AllocOp op, typename fhe::AllocOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op 
                                    << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        auto alignment = op.getAlignment();
        mlir::ArrayAttr args = rewriter.getArrayAttr({
            rewriter.getI64IntegerAttr(alignment.value())
        });

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, destTy, "Alloc", ValueRange{},
                                                         args, ArrayAttr{});
        return success();
    }
};

// Transform fhe::DeallocOp to emitc::CallOpaqueOp.
class FheDeallocPattern final : public OpConversionPattern<fhe::DeallocOp> {
  public:
    using OpConversionPattern<fhe::DeallocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::DeallocOp op, typename fhe::DeallocOp::Adaptor adaptor,
                                  ConversionPatternRewriter &rewriter) const override {
        auto destMemrefTy = typeConverter->convertType(op.getMemref().getType());
        if (!destMemrefTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op
                                    << ", the op type:" << op.getMemref().getType() << ".\n");
            return failure();
        }

        auto newMemref = typeConverter->materializeTargetConversion(rewriter, op.getMemref().getLoc(), destMemrefTy, op.getMemref());
        assert(newMemref);
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange{}, "Dealloc", newMemref);
        return success();
    }
};


void LowerFheToEmitcPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<func::FuncDialect>();
    registry.insert<memref::MemRefDialect>();
    registry.insert<emitc::EmitCDialect>();
    registry.insert<fhe::FHEDialect>();
}


void LowerFheToEmitcPass::runOnOperation() {
    auto type_converter = TypeConverter();

    // Type conversion, convert fhe types to emitc C++ types
    auto materializeCommon = [this](OpBuilder &builder, Type t, ValueRange vs, Location loc) -> std::optional<Value> {
        if (auto destTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto srcTy = vs.front().getType();
            if (mlir::dyn_cast_or_null<fhe::LWECipherType>(srcTy)) {
                if (destTy.getValue().str() == "LWECipher") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(srcTy)) {
                if (destTy.getValue().str() == "std::vector<LWECipher>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(srcTy)) {
                if (destTy.getValue().str() == "std::vector<std::vector<LWECipher>>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::RLWECipherType>(srcTy)) {
                if (destTy.getValue().str() == "RLWECipher") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::RLWECipherGridType>(srcTy)) {
                if (destTy.getValue().str() == "std::vector<RLWECipher>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::PlainType>(srcTy)) {
                if (destTy.getValue().str() == "Plain") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::PlainVectorType>(srcTy)) {
                if (destTy.getValue().str() == "PlainVector") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::PlainMatrixType>(srcTy)) {
                if (destTy.getValue().str() == "PlainMatrix") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::IntType>(srcTy)) {
                if (destTy.getValue().str() == "int") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            // deal with build-in type, the all following types mean clear types.
            else if (mlir::isa<MemRefType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            else if (mlir::isa<mlir::FloatType>(srcTy) || mlir::isa<mlir::IntegerType>(srcTy) || mlir::isa<mlir::IndexType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            else if (mlir::isa<mlir::VectorType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            // The global op type is emitc.array and needs to be converted to !emitc.opaque.
            else if (mlir::isa<emitc::ArrayType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            else {
                llvm::errs() << "[FheToEmitcPass] Materialization(materializeCommon) failed for type '" << srcTy << "\n";
            }
        } else {
            llvm::errs() << "[FheToEmitcPass] Materialization(materializeCommon) failed for type '" << t << "\n";
        }
 

        return std::optional<Value>(std::nullopt);
    };

    type_converter.addConversion([&](Type t) {
        if (mlir::isa<fhe::LWECipherType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "LWECipher"));
        }
        else if (mlir::isa<fhe::LWECipherVectorType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<LWECipher>"));
        }
        else if (mlir::isa<fhe::LWECipherMatrixType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<std::vector<LWECipher>>"));
        }
        else if (mlir::isa<fhe::RLWECipherType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "RLWECipher"));
        }
        else if (mlir::isa<fhe::RLWECipherGridType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<RLWECipher>"));
        }
        else if (mlir::isa<fhe::PlainType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "Plain"));
        }
        else if (mlir::isa<fhe::PlainVectorType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "PlainVector"));
        }
        else if (mlir::isa<fhe::PlainMatrixType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "PlainMatrix"));
        }
        else if (mlir::isa<fhe::IntType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "int"));
        }
        // deal with mlir build-in type, the all following types mean clear types.
        else if (mlir::isa<MemRefType>(t)) {
            auto newTy = mlir::cast<MemRefType>(t);
            if (newTy.hasStaticShape() && newTy.getShape().size() == 0) {
                return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "PlainVector")); // rank:0
            }
            else if (newTy.hasStaticShape() && newTy.getShape().size() == 1) {
                return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "PlainVector"));
            }
            else if (newTy.hasStaticShape() && newTy.getShape().size() == 2) {
                return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "PlainMatrix"));
            }
            else {
                llvm::errs() << "Unknow support rank is:" << newTy.getShape().size() << ".\n";
                return std::optional<Type>(std::nullopt);
            }
        }
        else if (mlir::isa<mlir::FloatType>(t) || mlir::isa<mlir::IntegerType>(t) ||
                 mlir::isa<mlir::IndexType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "Plain"));
        }
        else if (mlir::isa<mlir::VectorType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "PlainVector"));
        }

        LLVM_DEBUG(llvm::dbgs() << "Warning: No conver type:(" << t << ")[at FheToEmitcPass addConversion].\n");
        return std::optional<Type>(t);
    
    });

    type_converter.addTargetMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        return materializeCommon(builder, t, vs, loc);
    });

    type_converter.addArgumentMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        return materializeCommon(builder, t, vs, loc);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (mlir::isa<fhe::LWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "LWECipher") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::LWECipherVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "std::vector<LWECipher>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::LWECipherMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "std::vector<std::vector<LWECipher>>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::RLWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "RLWECipher") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::RLWECipherGridType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "std::vector<RLWECipher>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::PlainType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "Plain") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::PlainVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "PlainVector") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::PlainMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "PlainMatrix") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::IntType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "int") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        // deal with mlir build-in type, the all following types mean clear types.
        else if (mlir::isa<MemRefType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
        }
        else if (mlir::isa<mlir::FloatType>(t) || mlir::isa<mlir::IntegerType>(t) ||
                 mlir::isa<mlir::IndexType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
        }
        else if (mlir::isa<mlir::VectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
        }

        llvm::errs() << "[FheToEmitcPass] Materialization(addSourceMaterialization) failed for type '" << t << "\n";
        return std::optional<Value>(std::nullopt);
    });

    // Convert fhe op to emitc op target
    ConversionTarget target(getContext());
    target.addIllegalDialect<fhe::FHEDialect>();
    target.addIllegalOp<func::CallOp>();
    target.addIllegalOp<arith::ConstantOp>();
    target.addIllegalOp<memref::LoadOp, memref::GetGlobalOp, memref::GlobalOp>();
    target.addLegalOp<fhe::CastOp>();
    target.addLegalDialect<emitc::EmitCDialect>();
    target.addLegalOp<ModuleOp>();
    target.addDynamicallyLegalOp<func::FuncOp>([&](Operation *op) {
        auto funcOp = llvm::dyn_cast<func::FuncOp>(op);
        for (auto t : op->getOperandTypes()) {
            if (!type_converter.isLegal(t))
                return false;
        }
        for (auto t : op->getResultTypes()) {
            if (!type_converter.isLegal(t))
                return false;
        }
        for (auto t : funcOp.getFunctionType().getInputs()) {
            if (!type_converter.isLegal(t))
                return false;
        }
        for (auto t : funcOp.getFunctionType().getResults()) {
            if (!type_converter.isLegal(t))
                return false;
        }

        return true;
    });
    target.addDynamicallyLegalOp<func::ReturnOp>([&](Operation *op) { 
        return type_converter.isLegal(op->getOperandTypes()); 
    });

    mlir::RewritePatternSet fhePats(&getContext());
    fhePats.add<FheConstantPattern,
            FheArithUnaryPattern<fhe::LWENegOp>, FheArithUnaryPattern<fhe::RLWENegOp>,
            FheArithBinaryPattern<fhe::LWEAddOp>, FheArithBinaryPattern<fhe::LWEAddPlainOp>,
            FheArithBinaryPattern<fhe::RLWEAddOp>, FheArithBinaryPattern<fhe::RLWEAddPlainOp>,
            FheArithBinaryPattern<fhe::LWESubOp>, FheArithBinaryPattern<fhe::LWESubPlainOp>,
            FheArithBinaryPattern<fhe::RLWESubOp>, FheArithBinaryPattern<fhe::RLWESubPlainOp>,
            FheArithBinaryPattern<fhe::LWEMulOp>, FheArithBinaryPattern<fhe::LWEMulPlainOp>, 
            FheArithBinaryPattern<fhe::RLWEMulOp>, FheArithBinaryPattern<fhe::RLWEMulPlainOp>,
            FheRotatePattern, FheBootstrapPattern,
            FheCmpPattern, FheSelectPattern,
            FheFuncPattern, FheRetPattern, FheCallPattern,
            MemrefGetGlobalPattern, MemrefGlobalPattern,
            NativeMemrefLoadPattern, FheLoadPattern, FheVloadPattern, FheStorePattern, FheVstorePattern, FheCopyPattern,
            FheAllocaPattern, FheAllocPattern, FheDeallocPattern>(type_converter, fhePats.getContext());

    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(fhePats)))) {
        signalPassFailure();
    }
}