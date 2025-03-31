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
template <typename OpType>
class FheArithUnaryPattern final : public OpConversionPattern<OpType>
{
protected:
    using OpConversionPattern<OpType>::typeConverter;

public:
    using OpConversionPattern<OpType>::OpConversionPattern;

    LogicalResult matchAndRewrite(OpType op, typename OpType::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        // Materialize the operand where necessary
        Value newOperand;
        auto operand = op.getOperand();
        auto operandDestTy = typeConverter->convertType(operand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << operand << ", the op type:" << operand.getType() << ".\n");
            return failure();
        }

        if (operand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
        }
        else {
            newOperand = operand;
        }

        // Build a series of calls to our custom function.
        std::string opName;
        if (std::is_same<OpType, fhe::LWENegOp>() || std::is_same<OpType, fhe::RLWENegOp>()) {
            opName = "Neg";
        }
        else {
            LLVM_DEBUG(llvm::dbgs() << "Unkown the Op:" << OpType::getOperationName() << "not handle.\n");
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), opName, 
                                    ArrayAttr(), ArrayAttr(), newOperand);
        return success();
    }
};


// Convert FHE arith binary op(add/sub/mul/...) operations into emitc::CallOp.
template <typename OpType>
class FheArithBinaryPattern final : public OpConversionPattern<OpType>
{
protected:
    using OpConversionPattern<OpType>::typeConverter;

public:
    using OpConversionPattern<OpType>::OpConversionPattern;

    LogicalResult matchAndRewrite(OpType op, typename OpType::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        // Materialize the operands where necessary
        llvm::SmallVector<Value> materialized_ops;
        for (auto operand : op.getOperands()) {
            auto operandDestTy = typeConverter->convertType(operand.getType());
            if (!operandDestTy) {
                LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << operand << ", the op type:" << operand.getType() << ".\n");
                return failure();
            }

            if (operand.getType() != operandDestTy) {
                auto newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
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
            LLVM_DEBUG(llvm::dbgs() << "Unkown the Op:" << OpType::getOperationName() << "not handle.\n");
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), opName, 
                                    ArrayAttr(), ArrayAttr(), materialized_ops);
        return success();
    }
};


// Transform fhe::RotateOp to emitc::CallOpaqueOp.
class FheRotatePattern final : public OpConversionPattern<fhe::RotateOp>
{
public:
    using OpConversionPattern<fhe::RotateOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::RotateOp op, typename fhe::RotateOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto resTy = getTypeConverter()->convertType(op.getType());
        if (!resTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        Value operand = op.getCipher();
        auto operandDestTy = typeConverter->convertType(operand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << operand << ", the op type:" << operand.getType() << ".\n");
            return failure();
        }

        Value newOperand;
        if (operand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, operand);
        }
        else {
            newOperand = operand;
        }

        auto rotIdxAttr = ArrayAttr::get(getContext(), 
                                { IntegerAttr::get(IndexType::get(getContext()),0),
                                rewriter.getSI32IntegerAttr(op.getI())});

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, resTy, "Rotate", rotIdxAttr, ArrayAttr(), newOperand);

        return success();
    }
};


// Transform fhe::CallOp to emitc::CallOpaqueOp.
class FheCallPattern final : public OpConversionPattern<func::CallOp>
{
public:
    using OpConversionPattern<func::CallOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::CallOp op, typename func::CallOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto resTy = getTypeConverter()->convertType(op.getResult(0).getType());
        if (!resTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getResult(0).getType() << ".\n");
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
                materialized_ops.push_back(newOperand);
            }
            else {
                materialized_ops.push_back(operand);
            }
        }

        auto funcName = op.getCallee();
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(resTy), funcName, ArrayAttr(), ArrayAttr(), materialized_ops);

        return success();
    }   
};


// This is essentially boilerplate code, 
// with nothing here that actually depends on the dialect being converted.
class FheFuncPattern final : public OpConversionPattern<func::FuncOp>
{
public:
    using OpConversionPattern<func::FuncOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::FuncOp op, typename func::FuncOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        TypeConverter::SignatureConversion signatureConv(op.getFunctionType().getNumInputs());
        SmallVector<Type> newResTypes;
        if (failed(typeConverter->convertTypes(op.getFunctionType().getResults(), newResTypes))) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getFunctionType().getResults() << ".\n");
            return failure();
        }
        if (typeConverter->convertSignatureArgs(op.getFunctionType().getInputs(), signatureConv).failed()) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getFunctionType().getInputs() << ".\n");
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


// This is essentially boilerplate code, 
// with nothing here that actually depends on the dialect being converted.
class FheRetPattern final : public OpConversionPattern<func::ReturnOp>
{
public:
    using OpConversionPattern<func::ReturnOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(func::ReturnOp op, typename func::ReturnOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        if (op->getNumOperands() != 1) {
            emitError(op->getLoc(), "Currently, only single value returns are supported.");
            return failure();
        }
        auto destTy = this->getTypeConverter()->convertType(op->getOperandTypes().front());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op->getOperandTypes().front() << ".\n");
            return failure();
        }

        if (auto _ = mlir::dyn_cast_or_null<emitc::OpaqueType>(destTy)) {
            rewriter.setInsertionPoint(op);
            auto castOp = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), destTy, op.getOperands());
            rewriter.replaceOpWithNewOp<func::ReturnOp>(op, castOp);

        }
        
        return success();
    }
};


// Transform memref::GetGlobalOp to emitc::GetGlobalOp
class MemrefGetGlobalPattern final : public OpConversionPattern<memref::GetGlobalOp>
{
protected:
    using OpConversionPattern<memref::GetGlobalOp>::typeConverter;

public:
    using OpConversionPattern<memref::GetGlobalOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::GetGlobalOp op, typename memref::GetGlobalOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        // auto resTy = getTypeConverter()->convertType(op.getType());
        // if (!resTy) {
        //     return rewriter.notifyMatchFailure(op.getLoc(), "cannot convert result type");
        // }
         auto resTy = emitc::ArrayType::get(getContext(), op.getType().getShape(), op.getType().getElementType());

        rewriter.replaceOpWithNewOp<emitc::GetGlobalOp>(op, resTy, adaptor.getNameAttr());

        return success();
    }
};


// Transform memref::GlobalOp to emitc::GlobalOp
class MemrefGlobalPattern final : public OpConversionPattern<memref::GlobalOp>
{
protected:
    using OpConversionPattern<memref::GlobalOp>::typeConverter;

public:
    using OpConversionPattern<memref::GlobalOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::GlobalOp op, typename memref::GlobalOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        if (!op.getType().hasStaticShape()) {
            return rewriter.notifyMatchFailure(op.getLoc(), "cannot transform global with dynamic shape");
        }

        if (op.getAlignment().value_or(1) > 1) {
            return rewriter.notifyMatchFailure(op.getLoc(), "global variable with alignment requirement is currently not supported");
        }

        // auto resTy = getTypeConverter()->convertType(op.getType());
        // if (!resTy) {
        //     return rewriter.notifyMatchFailure(op.getLoc(), "cannot convert global op result type");
        // }
        auto resTy = emitc::ArrayType::get(getContext(), op.getType().getShape(), op.getType().getElementType());

        SymbolTable::Visibility visibility = SymbolTable::getSymbolVisibility(op);
        if (visibility != SymbolTable::Visibility::Public && visibility != SymbolTable::Visibility::Private) {
            return rewriter.notifyMatchFailure(op.getLoc(), "only public and private visibility is currently supported");
        }

        // We are explicit in specifing the linkage because the default linkage
        // for constants is different in C and C++.
        bool staticSpecifier = visibility == SymbolTable::Visibility::Private;
        bool externSpecifier = !staticSpecifier;

        Attribute initialValue = adaptor.getInitialValueAttr();
        if (isa_and_present<UnitAttr>(initialValue))
            initialValue = {};

        rewriter.replaceOpWithNewOp<emitc::GlobalOp>(op, adaptor.getSymName(), resTy, initialValue, 
                                        externSpecifier, staticSpecifier, adaptor.getConstant());

        return success();
    }
};


// Transform arith::ConstantOp to emitc::ConstantOp
class FheConstantPattern final : public OpConversionPattern<arith::ConstantOp>
{
protected:
    using OpConversionPattern<arith::ConstantOp>::typeConverter;

public:
    using OpConversionPattern<arith::ConstantOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(arith::ConstantOp op, typename arith::ConstantOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        // Get arith::ConstOp interger value.
        double dVal;
        auto valueAttr = op.getValue(); 
        if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(valueAttr)) {
            dVal = intAttr.getInt();
        } else if (auto floatAttr = mlir::dyn_cast<mlir::FloatAttr>(valueAttr)) {
            dVal = floatAttr.getValueAsDouble();
        }

        // Combine emitc::OpaqueAttr using the value.
        emitc::OpaqueAttr emitcAttrVal;
        emitcAttrVal = emitc::OpaqueAttr::get(getContext(), ("MakePlain(" + std::to_string(dVal) + ")"));
        rewriter.replaceOpWithNewOp<emitc::ConstantOp>(op, TypeRange(destTy), emitcAttrVal);
        
        return success();
    }
};


// Transform memref::LoadOp to emitc::LoadOp.
// mark: current mlir version not support emitc::LoadOp.
// class MemrefLoadPattern final : public OpConversionPattern<memref::LoadOp>
// {
// public:
//     using OpConversionPattern<memref::LoadOp>::OpConversionPattern;

//     LogicalResult matchAndRewrite(memref::LoadOp op, typename memref::LoadOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
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
class NativeMemrefLoadPattern final : public OpConversionPattern<memref::LoadOp>
{
protected:
    using OpConversionPattern<memref::LoadOp>::typeConverter;

public:
    using OpConversionPattern<memref::LoadOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(memref::LoadOp op, typename memref::LoadOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto resTy = getTypeConverter()->convertType(op.getType());
        if (!resTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        Value memOperand = op.getMemref();
        auto operandDestTy = typeConverter->convertType(memOperand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << memOperand << ", the op type:" << memOperand.getType() << ".\n");
            return failure();
        }

        Value newOperand;
        if (memOperand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, memOperand);
        }
        else {
            newOperand = memOperand;
        }

        SmallVector<Value, 4> indices(op.getIndices().begin(), op.getIndices().end());
        SmallVector<Value, 8> operands;
        operands.push_back(newOperand);
        operands.append(indices.begin(), indices.end());
        auto newOp = rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, resTy, "Native_Load", operands);

        return success();
    }
};


// Transform fhe::LoadOp to emitc::CallOpaqueOp.
class FheLoadPattern final : public OpConversionPattern<fhe::LoadOp>
{
public:
    using OpConversionPattern<fhe::LoadOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::LoadOp op, typename fhe::LoadOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" << op.getType() << ".\n");
            return failure();
        }
        
        auto memOperand = op.getMemref();
        auto operandDestTy = typeConverter->convertType(memOperand.getType());
        if (!operandDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << memOperand << ", the op type:" << memOperand.getType() << ".\n");
            return failure();
        }

        Value newOperand;
        if (memOperand.getType() != operandDestTy) {
            newOperand = typeConverter->materializeTargetConversion(rewriter, op.getLoc(), operandDestTy, memOperand);
            if (auto castOp = mlir::dyn_cast_or_null<fhe::CastOp>(memOperand.getDefiningOp())) {
                if (auto castOp2 = mlir::dyn_cast_or_null<fhe::CastOp>(castOp.getOperand().getDefiningOp())) {
                    newOperand = castOp2.getOperand();
                }
            }
        }
        else {
            newOperand = memOperand;
        }

        SmallVector<Value, 4> indices(op.getIndices().begin(), op.getIndices().end());
        SmallVector<Value, 8> operands;
        operands.push_back(newOperand);
        operands.append(indices.begin(), indices.end());
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, destTy, "Load", operands);
        
        return success();
    }
};


// Transform fhe::Store to emitc::CallOpaqueOp.
class FheStorePattern final : public OpConversionPattern<fhe::StoreOp>
{
public:
    using OpConversionPattern<fhe::StoreOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::StoreOp op, typename fhe::StoreOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {        
        rewriter.setInsertionPoint(op);

        auto destMemrefTy = typeConverter->convertType(op.getMemref().getType());
        if (!destMemrefTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op.getMemref() << ", the op type:" 
                                    << op.getMemref().getType() << ".\n");
            return failure();
        }

        auto destStoreValTy = typeConverter->convertType(op.getValueToStore().getType());
        if (!destStoreValTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op.getValueToStore() << ", the op type:" 
                                    << op.getValueToStore().getType() << ".\n");
            return failure();
        }

        llvm::SmallVector<Value, 8> operands;
        llvm::SmallVector<Value, 4> indices(op.getIndices().begin(), op.getIndices().end());
        auto newMemref = typeConverter->materializeTargetConversion(rewriter, op.getMemref().getLoc(),
                                                                destMemrefTy, op.getMemref());
        auto newValueToStore = typeConverter->materializeTargetConversion(rewriter, op.getValueToStore().getLoc(),
                                                                destStoreValTy, op.getValueToStore());
      
        operands.push_back(newMemref);
        operands.push_back(newValueToStore);
        operands.append(indices.begin(), indices.end());
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, destMemrefTy, "Store", operands);

        return success();
    }
};


// Transform fhe::CopyOp to emitc::CallOpaqueOp.
class FheCopyPattern final : public OpConversionPattern<fhe::CopyOp>
{
public:
    using OpConversionPattern<fhe::CopyOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::CopyOp op, typename fhe::CopyOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        rewriter.setInsertionPoint(op);

        auto newSrcTy = typeConverter->convertType(op.getSource().getType());
        if (!newSrcTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op.getSource() << ", the op type:" 
                                    << op.getSource().getType() << ".\n");
            return failure();
        }

        auto newDestTy = typeConverter->convertType(op.getTarget().getType());
        if (!newDestTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op.getTarget() << ", the op type:" 
                                    << op.getTarget().getType() << ".\n");
            return failure();
        }

        auto newSrcVal = typeConverter->materializeTargetConversion(rewriter, op.getSource().getLoc(),
                                                                newSrcTy, op.getSource());
        auto newDestVal = typeConverter->materializeTargetConversion(rewriter, op.getTarget().getLoc(),
                                                                newDestTy, op.getTarget());
        llvm::SmallVector<Value, 8> operands;
        operands.push_back(newSrcVal);
        operands.push_back(newDestVal);
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange{}, "Copy", operands);

        return success();
    }
};


// Transform fhe::AllocaOp to emitc::CallOpaqueOp.
class FheAllocaPattern final : public OpConversionPattern<fhe::AllocaOp>
{
public:
    using OpConversionPattern<fhe::AllocaOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::AllocaOp op, typename fhe::AllocaOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op<< ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, destTy, "Alloca", ValueRange{});
        return success();
    }
};


// Transform fhe::AllocOp to emitc::CallOpaqueOp.
class FheAllocPattern final : public OpConversionPattern<fhe::AllocOp>
{
public:
    using OpConversionPattern<fhe::AllocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::AllocOp op, typename fhe::AllocOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destTy = typeConverter->convertType(op.getType());
        if (!destTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op<< ", the op type:" << op.getType() << ".\n");
            return failure();
        }

        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, destTy, "Alloc", ValueRange{});
        return success();
    }
};


// Transform fhe::DeallocOp to emitc::CallOpaqueOp.
class FheDeallocPattern final : public OpConversionPattern<fhe::DeallocOp>
{
public:
    using OpConversionPattern<fhe::DeallocOp>::OpConversionPattern;

    LogicalResult matchAndRewrite(fhe::DeallocOp op, typename fhe::DeallocOp::Adaptor adaptor, ConversionPatternRewriter &rewriter) const override
    {
        auto destMemrefTy = typeConverter->convertType(op.getMemref().getType());
        if (!destMemrefTy) {
            LLVM_DEBUG(llvm::dbgs() << "call convertType fail for op: " << op << ", the op type:" 
                                    << op.getMemref().getType() << ".\n");
            return failure();
        }

        auto newMemref = typeConverter->materializeTargetConversion(rewriter, op.getMemref().getLoc(),
                                                                destMemrefTy, op.getMemref());
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange{}, "Dealloc", newMemref);
        return success();
    }
};


void LowerFheToEmitcPass::getDependentDialects(mlir::DialectRegistry &registry) const
{
    registry.insert<func::FuncDialect>();
    registry.insert<memref::MemRefDialect>();
    registry.insert<emitc::EmitCDialect>();
    registry.insert<fhe::FHEDialect>();
}


void LowerFheToEmitcPass::runOnOperation()
{
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
                if (destTy.getValue().str() == "std::vector<Plain>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::PlainMatrixType>(srcTy)) {
                if (destTy.getValue().str() == "std::vector<std::vector<Plain>>") {
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
            // The global op type is emitc.array and needs to be converted to !emitc.opaque.
            else if (mlir::isa<emitc::ArrayType>(srcTy)) {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
            }
            else {
                llvm::outs() << "Warning:No handling for the ValueRange type:" << srcTy <<"[at FheToEmitcPass materializeCommon].\n";
            }
        }
 
        llvm::outs() << "No handling for the type:(" << t << ")[at FheToEmitcPass materializeCommon].\n";
        LLVM_DEBUG(llvm::dbgs() << "No handling for the type:(" << t << ")[at FheToEmitcPass materializeCommon].\n");
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
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<Plain>"));
        }
        else if (mlir::isa<fhe::PlainMatrixType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<std::vector<Plain>>"));
        }
        else if (mlir::isa<fhe::IntType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "int"));
        }
        // deal with mlir build-in type, the all following types mean clear types.
        else if (mlir::isa<MemRefType>(t)) {
            auto newTy = mlir::cast<MemRefType>(t);
            if (newTy.hasStaticShape() && newTy.getShape().size() == 0) {
                return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<Plain>")); // rank:0
            }
            else if (newTy.hasStaticShape() && newTy.getShape().size() == 1) {
                return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<Plain>"));
            }
            else if (newTy.hasStaticShape() && newTy.getShape().size() == 2) {
                return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "std::vector<std::vector<Plain>>"));
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

        llvm::outs() << "Warning: No conver type:(" << t << ")[at FheToEmitcPass addConversion].\n";
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
                if (srcTy.getValue().str() == "std::vector<Plain>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, t, vs));
                }
            }
        }
        else if (mlir::isa<fhe::PlainMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "std::vector<std::vector<Plain>>") {
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

        llvm::outs() << "Warning: No handling for the type:(" << t << ")[at FheToEmitcPass addSourceMaterialization].\n";
        LLVM_DEBUG(llvm::dbgs() << "No handling for the type:(" << t << ")[at FheToEmitcPass addSourceMaterialization].\n");
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
            FheRotatePattern,
            FheFuncPattern, FheRetPattern, FheCallPattern,
            MemrefGetGlobalPattern, MemrefGlobalPattern,
            NativeMemrefLoadPattern, FheLoadPattern, FheStorePattern, FheCopyPattern,
            FheAllocaPattern, FheAllocPattern, FheDeallocPattern>(type_converter, fhePats.getContext());

    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(fhePats)))) {
        signalPassFailure();
    }
}