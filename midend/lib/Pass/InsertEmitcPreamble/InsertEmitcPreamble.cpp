#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Conversion/MemRefToEmitC/MemRefToEmitC.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/include/mlir/IR/MLIRContext.h"            
#include "mlir/include/mlir/IR/PatternMatch.h"           
#include "Pass/InsertEmitcPreamble/InsertEmitcPreamble.h"

#define DEBUG_TYPE "insert-emitc-preamble"

using namespace mlir;

void InsertEmitcPreamblePass::getDependentDialects(mlir::DialectRegistry &registry) const
{
    registry.insert<emitc::EmitCDialect>();
}


void InsertEmitcPreamblePass::runOnOperation() 
{
    ModuleOp module = getOperation();
    OpBuilder builder(module.getContext());

    // Define the content to be inserted for IncludeOp and VerbatimOp.
    SmallVector<std::pair<StringRef, bool>> incLines = {
        {"vector",    /*isSystem=*/true},
        {"openfhe.h", /*isSystem=*/false}
    };

    SmallVector<StringRef> verbatimLines = {
        "using namespace std;",
        "using namespace lbcrypto;",
        "using CiphertextT = ConstCiphertext<DCRTPoly>;",
        "using PlaintextT = Plaintext;",
        "using MutableCiphertextT = Ciphertext<DCRTPoly>;",
        "using CCParamsT = CCParams<CryptoContext{0}RNS>;",
        "using CryptoContextT = CryptoContext<DCRTPoly>;",
        "using EvalKeyT = EvalKey<DCRTPoly>;",
        "using PrivateKeyT = PrivateKey<DCRTPoly>;",
        "using PublicKeyT = PublicKey<DCRTPoly>;",
    };

    // Traverse all ModelOp instances and insert emitc::IncludeOp and emitc::VerbatimOp before each of them.
    module.walk([&](Operation *op) {
        // if (op->getName().getStringRef() != "buildin.model") {
        //     return;
        // }

        Block *block = op->getBlock();
        builder.setInsertionPoint(op);

        // Insert all emitc::IncludeOp
        for (auto &include : incLines) {
            auto includeOp = builder.create<emitc::IncludeOp>(op->getLoc(), include.first, include.second);
            includeOp->moveBefore(op);
        }

        // Insert all VerbatimOp
        for (auto &line : verbatimLines) {
            auto verbatimOp = builder.create<emitc::VerbatimOp>(op->getLoc(), line);
            verbatimOp->moveBefore(op);
        }
    });
}