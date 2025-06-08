#include <memory>
#include <iostream>

#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Conversion/MemRefToEmitC/MemRefToEmitC.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/include/mlir/IR/MLIRContext.h"            
#include "mlir/include/mlir/IR/PatternMatch.h"           
#include "mlir/include/mlir/IR/Value.h"     
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "Dialect/Secret/SecretDialect.h"
#include "Dialect/Secret/SecretOps.h"
#include "Dialect/Secret/SecretTypes.h"
#include "Common/FheDefines.h"
#include "Pass/CastToEmitcStub/LowerCastToEmitcStub.h"

#define DEBUG_TYPE "cast-to-emitc-stub"

using namespace mlir;
using namespace aegis;
using namespace aegis::fhe;
using namespace aegis::secret;


static bool isOpaqueRLWE(Type type) {
    return mlir::isa<emitc::OpaqueType>(type) && 
                (mlir::cast<emitc::OpaqueType>(type).getValue() == RLWECIPHER_TYPE_NAME||
                 mlir::cast<emitc::OpaqueType>(type).getValue() == VECT_RLWECIPHER_TYPE_NAME);
}

static int32_t getElementSizes(Type type) {
    if (auto operandTy = mlir::dyn_cast_or_null<fhe::LWECipherVectorType>(type)) {
        return operandTy.getSize();
    } else if (auto operandTy = mlir::dyn_cast_or_null<fhe::RLWECipherType>(type)) {
        return operandTy.getSize();
    } else {
        return 1;
    }
}

// CastPattern replace fhe::CastOp to a dummy emitc function call. 
class CastPattern : public OpRewritePattern<fhe::CastOp> {
public:
    using OpRewritePattern<fhe::CastOp>::OpRewritePattern;

    LogicalResult matchAndRewrite(fhe::CastOp op, PatternRewriter &rewriter) const override {
        auto destTy = op.getType();
        auto operand = op.getOperand();

        if (auto constantOp = mlir::dyn_cast_or_null<emitc::ConstantOp>(operand.getDefiningOp())) {
            // Get value attribute
            Attribute valueAttr = constantOp.getValueAttr();

            // Exist OpaqueAttr ?
            if (auto opaqueAttr = mlir::dyn_cast<emitc::OpaqueAttr>(valueAttr)) {
                StringRef valueStr = opaqueAttr.getValue();
                if (valueStr.starts_with("MakePlain")) {
                    // plain to index
                    if (mlir::isa<mlir::IndexType>(destTy)) {
                        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "Cast_Plain_To_Index", 
                                        ArrayAttr(), ArrayAttr(), operand);
                        return success();
                    // plain to cipher
                    } else if (mlir::isa<emitc::OpaqueType>(destTy) && 
                               mlir::cast<emitc::OpaqueType>(destTy).getValue() == RLWECIPHER_TYPE_NAME) {
                        rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "Cast_Plain_To_Cipher", 
                                        ArrayAttr(), ArrayAttr(), operand);
                        return success();
                    } else {
                        llvm::errs() << "The conversion of plaintext to the type(" << destTy << ")has not been processed yet." << "\n";
                        return failure();
                    }
                } else if (mlir::isa<mlir::FloatType, mlir::IntegerType, mlir::IndexType>(destTy)) {
                    // maybe from plain to cipher, eg:select x, 1, 0, At this point, 
                    // we cannot replace it and should simply return.
                    return success(); 
                } else {
                    llvm::errs() << "Cast_Plain_To_Index requires 'MakePlain' prefix and IndexType, but got "
                                 << valueStr  << " and destTy " << destTy << "\n";
                    return failure(); 
                }
            } else {
                llvm::errs() << "Expected OpaqueAttr but got " << valueAttr << " for operation " << op << "\n";
                return failure(); 
            }
        } else if (auto getGlobalOp = mlir::dyn_cast_or_null<emitc::GetGlobalOp>(operand.getDefiningOp())) {
            rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "ConstantArray_to_PlainVector", 
                                                             ArrayAttr(), ArrayAttr(), operand);
            return success();
        } else if (op->getAttrOfType<IntegerAttr>("from_params") &&
                   op->getAttrOfType<IntegerAttr>("to_params")) {
            rewriter.replaceOpWithNewOp<emitc::CallOpaqueOp>(op, TypeRange(destTy), "Cast_Stub", 
                                        ArrayAttr(), ArrayAttr(), operand);
            return success();
        } else {
            auto optimizeCastChain = [&](fhe::CastOp op, PatternRewriter &rewriter) -> LogicalResult {
                // Check if CastOp operand type is not RLWE opaque type, and dest type is RLWE opaque type,
                // if this condition is not met, return directly. (bottom-up search)
                /*********************************************************************
                // case 1
                // original operation chain:
                %1 = fhe.cast(%0) : !opaque -> !rlwe<4>  
                %2 = fhe.cast(%1) : !rlwe<4> -> !lwe<4>
                %3 = fhe.cast(%2) : !lwe<4> -> !lwe<16>
                %4 = fhe.cast(%3) : !lwe<16> -> !rlwe<16>
                %5 = fhe.cast(%4) : !rlwe<16> -> !opaque

                // After optimization:
                %5 = fhe.cast(%arg1) {from_params = 4 : i64, to_params = 16 : i64} : 
                                (!emitc.opaque<"RLWECipher">) -> !emitc.opaque<"RLWECipher">
                **********************************************************************/
                /*********************************************************************
                // case 2(arith.select %cmp, %one, %zero)
                // original operation chain:
                %6 = fhe.cast(%4) : (!emitc.opaque<"Plain">) -> f64
                %12 = "secret.cast"(%6) : (f64) -> !secret.secret<f64>
                %14 = fhe.cast(%12) : (!secret.secret<f64>) -> !fhe.lwe<f64>
                %16 = fhe.cast(%14) : (!fhe.lwe<f64>) -> !fhe.rlwe<1 x f64>
                %18 = fhe.cast(%16) : (!fhe.rlwe<1 x f64>) -> !emitc.opaque<"RLWECipher">

                // After optimization:
                %5 = fhe.cast(%arg1) {from_params = 4 : i64, to_params = 16 : i64} : 
                                (!emitc.opaque<"Plain">) -> !emitc.opaque<"RLWECipher">
                **********************************************************************/
                // TODO: need ResizeOp replace the fhe.cast Op?
                auto root = op.getOperand();
                if (!(!isOpaqueRLWE(root.getType()) && isOpaqueRLWE(op.getType()))) {
                    return failure();
                }

                // Collect intermediate cast chains
                // SmallVector<fhe::CastOp> castChain;
                // while (auto prevCast = root.getDefiningOp<fhe::CastOp>()) {
                //     if (!prevCast->hasOneUse()) {
                //         break;
                //     }
                //     castChain.push_back(prevCast);
                //     root = prevCast.getOperand();
                // }
                SmallVector<mlir::Operation*> castChain;
                while (true) {
                    mlir::Operation* currentOp = root.getDefiningOp();
                    if (currentOp == nullptr) 
                        break;
                    
                    if (auto fheCast = llvm::dyn_cast_or_null<fhe::CastOp>(currentOp)) {
                        if (!fheCast->hasOneUse()) {
                            break;
                        }
                        castChain.push_back(fheCast);
                        root = fheCast.getOperand();
                    } else if (auto secretCast = llvm::dyn_cast_or_null<secret::CastOp>(currentOp)) {
                        if (!secretCast->hasOneUse()) {
                            break;
                        }
                        castChain.push_back(secretCast);
                        root = secretCast.getOperand();
                    } else {
                        break;
                    }
                }

                // Verify the validity of the transformation chain
                if (castChain.empty()) {
                    return failure();
                }

                auto firstCastOp = mlir::dyn_cast<fhe::CastOp>(castChain.front());
                auto lastCastOp  = mlir::dyn_cast<fhe::CastOp>(castChain.back());
                assert(firstCastOp && lastCastOp);
                int64_t toDim = getElementSizes(firstCastOp.getType());
                int64_t fromDim = getElementSizes(lastCastOp.getType());
                // llvm::outs() << "toDim type:" << castChain.front().getType() << "\n";
                // llvm::outs() << "fromDim type:" << castChain.back().getType() << "\n";
                assert(fromDim>0 && toDim>0);

                // Create a new cast operation and replace
                //auto newCast = rewriter.create<fhe::CastOp>(op.getLoc(), op.getType(), operand);
                OperationState state(op.getLoc(), fhe::CastOp::getOperationName());
                state.addOperands(root);
                state.addTypes(op.getType());

                // Create attributes
                auto fromAttr = rewriter.getI64IntegerAttr(fromDim);
                auto toAttr = rewriter.getI64IntegerAttr(toDim);
                state.addAttribute("from_params", fromAttr);
                state.addAttribute("to_params", toAttr);
                auto newCast = mlir::cast<fhe::CastOp>(rewriter.create(state));
                // auto fromDim = newCast->getAttrOfType<IntegerAttr>("from_params").getInt();
                // auto toDim = newCast->getAttrOfType<IntegerAttr>("to_params").getInt();

                rewriter.replaceOp(op, newCast);
                
                // Clean up old cast ops
                for (auto *castOp : castChain) {
                    rewriter.eraseOp(castOp);
                }

                return success();
            };

            return optimizeCastChain(op, rewriter);
        }
    }
};


void LowerCastToEmitcStubPass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<fhe::FHEDialect>();
}


void LowerCastToEmitcStubPass::runOnOperation()  {
    mlir::RewritePatternSet patterns(&getContext());
    patterns.add<CastPattern>(&getContext());
    (void)applyPatternsAndFoldGreedily(getOperation(), std::move(patterns));
}
