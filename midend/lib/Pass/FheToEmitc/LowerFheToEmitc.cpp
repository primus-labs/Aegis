#include <string>

#include "llvm/ADT/APSInt.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Transforms/DialectConversion.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Pass/FheToEmitc/LowerFheToEmitc.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "fhe-to-emitc"

using namespace mlir;
using namespace aegis;
using namespace fhe;


// Convert FHE add/sub/mul/... operations into emitc::CallOp 
// to facilitate further transformation into C++ functions.
template <typename OpType>
class FheArithBasicPattern final : public OpConversionPattern<OpType>
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
        if (std::is_same<OpType, LWEAddOp>()) {
            opName = "Add";
        }
        else if (std::is_same<OpType, LWESubOp>()) {
            opName = "Sub";
        }
        else if (std::is_same<OpType, LWEMulOp>() ||
                 std::is_same<OpType, RLWEMulOp>()) {
            opName = "Mul";
        }
        else {
            LLVM_DEBUG(llvm::dbgs() << "Unkown this Op, not handle.\n");
            return failure();
        }
        
        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), opName, 
                                    ArrayAttr(), ArrayAttr(), materialized_ops);
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





void LowerFheToEmitcPass::getDependentDialects(mlir::DialectRegistry &registry) const
{
    registry.insert<func::FuncDialect>();
    registry.insert<mlir::emitc::EmitCDialect>();
}


void LowerFheToEmitcPass::runOnOperation()
{
    auto type_converter = TypeConverter();

    // Type conversion, convert fhe types to emitc C++ types
    auto materializeCommon = [](OpBuilder &builder, Type t, ValueRange vs, Location loc) -> std::optional<Value> {
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
            else if (mlir::dyn_cast_or_null<fhe::RLWECipherType>(srcTy)) {
                if (destTy.getValue().str() == "std::vector<RLWECipher>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::PlainType>(srcTy)) {
                if (destTy.getValue().str() == "Plain") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
            else if (mlir::dyn_cast_or_null<fhe::IntType>(srcTy)) {
                if (destTy.getValue().str() == "int") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }

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
        else if (mlir::isa<fhe::IntType>(t)) {
            return std::optional<Type>(emitc::OpaqueType::get(&getContext(), "int"));
        }
        else {
            return std::optional<Type>(t);
        }
    });

    type_converter.addTargetMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        return materializeCommon(builder, t, vs, loc);
    });

    type_converter.addArgumentMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        return materializeCommon(builder, t, vs, loc);
    });

    type_converter.addSourceMaterialization([&](OpBuilder &builder, Type t, ValueRange vs, Location loc) {
        if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "LWECipher") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "std::vector<LWECipher>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::LWECipherMatrixType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto ot = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (ot.getValue().str() == "std::vector<std::vector<LWECipher>>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::RLWECipherType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "RLWECipher") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::RLWECipherGridType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto srcTy = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (srcTy.getValue().str() == "std::vector<RLWECipher>") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::PlainType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto ot = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (ot.getValue().str() == "Plain") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }
        else if (auto destTy = mlir::dyn_cast_or_null<fhe::IntType>(t)) {
            assert(!vs.empty() && ++vs.begin() == vs.end() && "currently can only materialize single values");
            if (auto ot = mlir::dyn_cast_or_null<emitc::OpaqueType>(vs.front().getType())) {
                if (ot.getValue().str() == "int") {
                    return std::optional<Value>(builder.create<fhe::CastOp>(loc, destTy, vs));
                }
            }
        }

        LLVM_DEBUG(llvm::dbgs() << "No handling for the type:(" << t << ")[at FheToEmitcPass addSourceMaterialization].\n");
        return std::optional<Value>(std::nullopt);
    });


    ConversionTarget target(getContext());
    target.addIllegalDialect<fhe::FHEDialect>();
    target.addIllegalOp<func::CallOp>();
    target.addIllegalOp<arith::ConstantOp>();
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
    fhePats.add<FheArithBasicPattern<LWEAddOp>, FheArithBasicPattern<LWESubOp>, 
            FheArithBasicPattern<LWEMulOp>, FheArithBasicPattern<RLWEMulOp>,
            FheFuncPattern, FheRetPattern, FheCallPattern>(type_converter, fhePats.getContext());

    if (mlir::failed(mlir::applyPartialConversion(getOperation(), target, std::move(fhePats)))) {
        signalPassFailure();
    }
}