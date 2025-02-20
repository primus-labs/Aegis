#include <queue>
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/Sequence.h"
#include "Pass/lwe2rlwe/LowerLweToRlwe.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"

using namespace mlir;
using namespace aegis;
using namespace fhe;


void LweToRlwePass::getDependentDialects(mlir::DialectRegistry &registry) const
{
  registry.insert<fhe::FHEDialect,
                  mlir::affine::AffineDialect,
                  func::FuncDialect,
                  mlir::scf::SCFDialect>();
}


// Transform the batched LWE addition operator into RLWE addition
LogicalResult LweAddToRlweOperation(IRRewriter &rewriter, MLIRContext *context, LWEAddOp op, TypeConverter typeConverter)
{
    rewriter.setInsertionPoint(op);

    auto dstType = typeConverter.convertType(op.getType());
    if (!dstType)
        return failure();
    
    llvm::SmallVector<Value> materialized_operands;
    for (Value o : op.getOperands())
    {
        auto operandDstType = typeConverter.convertType(o.getType());
        if (!operandDstType)
            return failure();
        if (o.getType() != operandDstType) {
            auto new_operand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), operandDstType, o);
            assert(new_operand && "Type Conversion must not fail");
            materialized_operands.push_back(new_operand);
        }
        else {
            materialized_operands.push_back(o);
        }      
    }

    rewriter.replaceOpWithNewOp<LWEAddOp>(op, dstType, materialized_operands);
    return success();
}

// Transform the batched LWE substraction operator into RLWE substraction
LogicalResult LweSubToRlweOperation(IRRewriter &rewriter, MLIRContext *context, LWESubOp op, TypeConverter typeConverter)
{
    rewriter.setInsertionPoint(op);

    auto dstType = typeConverter.convertType(op.getType());
    if (!dstType)
        return failure();
    
    llvm::SmallVector<Value> materialized_operands;
    for (Value o : op.getOperands())
    {
        auto operandDstType = typeConverter.convertType(o.getType());
        if (!operandDstType)
            return failure();
        if (o.getType() != operandDstType) {
            auto new_operand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), operandDstType, o);
            assert(new_operand && "Type Conversion must not fail");
            materialized_operands.push_back(new_operand);
        }
        else {
            materialized_operands.push_back(o);
        }
    }
    
    rewriter.replaceOpWithNewOp<LWESubOp>(op, dstType, materialized_operands);
    return success();
    
}

// Transform the batched LWE multiplication operator into RLWE multiplication
LogicalResult LweMulToRlweOperation(IRRewriter &rewriter, MLIRContext *context, LWEMulOp op, TypeConverter typeConverter)
{
    rewriter.setInsertionPoint(op);

    auto dstType = typeConverter.convertType(op.getType());
    if (!dstType)
        return failure();
    
    llvm::SmallVector<Value> materialized_operands;
    for (Value o : op.getOperands())
    {
        auto operandDstType = typeConverter.convertType(o.getType());
        if (!operandDstType)
            return failure();
        if (o.getType() != operandDstType) {
            auto new_operand = typeConverter.materializeTargetConversion(rewriter, op.getLoc(), operandDstType, o);
            assert(new_operand && "Type Conversion must not fail");
            materialized_operands.push_back(new_operand);
        }
        else {
            materialized_operands.push_back(o);
        }
    }
    
    rewriter.replaceOpWithNewOp<LWEMulOp>(op, dstType, materialized_operands);
    return success();
    
}


// Tranform a program computed in pure LWE ciphertexts into RLWE ciphertexts after batching optimizations
void LweToRlwePass::runOnOperation()
{
    auto type_converter = TypeConverter();

    type_converter.addConversion([&](Type t) {
        if (mlir::isa<LWECipherVectorType>(t))
        {
            int size = -155;
            auto new_t = mlir::cast<LWECipherVectorType>(t);
            size = new_t.getSize();
            return std::optional<Type>(RLWECipherType::get(&getContext(), new_t.getPlaintextType(), size));
        }
        else
            return std::optional<Type>(t);
    });
    type_converter.addTargetMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto ot = mlir::dyn_cast_or_null<RLWECipherType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<LWECipherVectorType>(old_type))
            {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, ot, vs));
            }
        }
        return std::optional<Value>(std::nullopt);
    });
    type_converter.addArgumentMaterialization([&] (OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto ot = mlir::dyn_cast_or_null<RLWECipherType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materalize single values");
            auto old_type = vs.front().getType();
            if (mlir::dyn_cast_or_null<LWECipherVectorType>(old_type))
            {
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, ot, vs));
            }
        }
        return std::optional<Value>(std::nullopt);
    });
    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto bst = mlir::dyn_cast_or_null<LWECipherVectorType>(t))
        {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            auto old_type = vs.front().getType();
            if (auto ot = mlir::dyn_cast_or_null<RLWECipherType>(old_type))
                return std::optional<Value>(builder.create<fhe::CastOp>(loc, bst, vs));
        }
        return std::optional<Value>(std::nullopt);
    });

    
    auto &block = getOperation()->getRegion(0).getBlocks().front();
    IRRewriter rewriter(&getContext());

    for (auto f : llvm::make_early_inc_range(block.getOps<func::FuncOp>()))
    {
        if (f.walk([&](Operation *op) {
            if (LWESubOp sub_op = llvm::dyn_cast_or_null<LWESubOp>(op)) {
                if (LweSubToRlweOperation(rewriter, &getContext(), sub_op, type_converter).failed())
                return WalkResult::interrupt();
            } 
            else if (LWEAddOp add_op = llvm::dyn_cast_or_null<LWEAddOp>(op)) {
                if (LweAddToRlweOperation(rewriter, &getContext(), add_op, type_converter).failed())
                return WalkResult::interrupt();
            } 
            else if (LWEMulOp mul_op = llvm::dyn_cast_or_null<LWEMulOp>(op)) {
                if (LweMulToRlweOperation(rewriter, &getContext(), mul_op, type_converter).failed())
                return WalkResult::interrupt();
            } 
            return WalkResult(success()); })
                .wasInterrupted())
        signalPassFailure();

        func::FuncOp op = f;
        TypeConverter::SignatureConversion signatureConversion(op.getFunctionType().getNumInputs());
        SmallVector<Type> newResultTypes;
        if (failed(type_converter.convertTypes(op.getFunctionType().getResults(), newResultTypes)))
            signalPassFailure();
        if (type_converter.convertSignatureArgs(op.getFunctionType().getInputs(), signatureConversion).failed())
            signalPassFailure();
        auto new_functype = FunctionType::get(&getContext(), signatureConversion.getConvertedTypes(), newResultTypes);

        rewriter.startOpModification(op);
        op.setType(new_functype);
        for (auto it = op.getRegion().args_begin(); it != op.getRegion().args_end(); ++it)
        {
            auto arg = *it;
            auto oldType = arg.getType();
            auto newType = type_converter.convertType(oldType);
            arg.setType(newType);
            if (newType != oldType)
            {
                rewriter.setInsertionPointToStart(&op.getBody().getBlocks().front());
                auto m_op = type_converter.materializeSourceConversion(rewriter, arg.getLoc(), oldType, arg);
                // llvm::errs()<<"\n"<<m_op<<"\n";
                arg.replaceAllUsesExcept(m_op, m_op.getDefiningOp());
            }
        }

        for (auto &block : op.getBody()) {
            for (auto returnOp : llvm::make_early_inc_range(block.getOps<func::ReturnOp>())) {
                SmallVector<Value, 4> newOperands;
                for (auto operand : returnOp.getOperands()) {
                    auto oldType = operand.getType();
                    auto newType = type_converter.convertType(oldType);
                    if (newType != oldType) {
                        rewriter.setInsertionPoint(returnOp);
                        auto convertedOperand = type_converter.materializeTargetConversion(
                            rewriter, returnOp.getLoc(), newType, operand);
                        if (!convertedOperand) {
                            emitError(returnOp.getLoc(), "Failed to convert return operand type");
                            signalPassFailure();
                        }
                        newOperands.push_back(convertedOperand);
                    } else {
                        newOperands.push_back(operand);
                    }
                }
                rewriter.eraseOp(returnOp);
                rewriter.setInsertionPointToEnd(&block);
                rewriter.create<func::ReturnOp>(returnOp.getLoc(), newOperands);
            }
        }
        rewriter.finalizeOpModification(op);
    }
}