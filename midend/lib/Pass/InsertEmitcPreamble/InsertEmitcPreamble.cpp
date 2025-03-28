#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Conversion/MemRefToEmitC/MemRefToEmitC.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
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

    SmallVector<StringRef> verbatimUsing = {
        "using namespace std;",
        "using namespace lbcrypto;",
        "using CiphertextT = Ciphertext<DCRTPoly>;",
        "using RLWECipher = Ciphertext<DCRTPoly>;",
        "using LWECipher = Ciphertext<DCRTPoly>;",
        "using PlaintextT = Plaintext;",
        "using MutableCiphertextT = Ciphertext<DCRTPoly>;",
        "using CCParamsT = CCParams<CryptoContextCKKSRNS>;",
        "using CryptoContextT = CryptoContext<DCRTPoly>;",
        "using EvalKeyT = EvalKey<DCRTPoly>;",
        "using PrivateKeyT = PrivateKey<DCRTPoly>;",
        "using PublicKeyT = PublicKey<DCRTPoly>;",
    };

    SmallVector<StringRef> verbatimMacros = {
        "#define Add(a, b) cryptoCtx->EvalAdd((a), (b))",
        "#define Mul(a, b) cryptoCtx->EvalMult((a), (b))",
    };

    // TODO, We must dynamically generate the corresponding encryption parameters based on the program.
    SmallVector<StringRef> verbatimInitCC = {
        "CryptoContext<DCRTPoly> cryptoCtx;",
        "void init_cryptcontext() {",
        "   CCParams<CryptoContextBGVRNS> parameters;",
        "   //TODO",
        "   cryptoCtx = GenCryptoContext(parameters);",
        "   cryptoCtx->Enable(PKE);",
        "   cryptoCtx->Enable(KEYSWITCH);",
        "   cryptoCtx->Enable(LEVELEDSHE);",
        "}",
    };

    // Traverse all ModuleOp instances and insert emitc::IncludeOp and emitc::VerbatimOp before each of them.
    // Terminate traversal after finding the first one.
    module.walk([&](mlir::ModuleOp op) {
        // Ensure the insertion point is in the top-level block of the module.
        auto moduleBlock = op.getBody()->front().getBlock();
        builder.setInsertionPointToStart(moduleBlock);

        // Insert all emitc::IncludeOp
        for (auto &include : incLines) {
            builder.create<emitc::IncludeOp>(op->getLoc(), include.first, include.second);
        }

        // Insert all using stmts
        for (auto &stmt : verbatimUsing) {
            builder.create<emitc::VerbatimOp>(op->getLoc(), stmt);
        }

        // Insert all macros
        for (auto &mac : verbatimMacros) {
            builder.create<emitc::VerbatimOp>(op->getLoc(), mac);
        }

        // Insert init cryptcontext function
        for (auto &stmt : verbatimInitCC) {
            builder.create<emitc::VerbatimOp>(op->getLoc(), stmt);
        }

        return mlir::WalkResult::interrupt();
    });

    // Traverse all functions in the module and 
    // Insert a call to the init_cryptcontext function at the beginning of target function.
    module.walk([&](func::FuncOp funcOp) {
        // Only process the target function
        // if (funcOp.getName() != "main") {
        //     return;
        // }

        // Get the entry block of the function
        Block &entryBlock = funcOp.getBody().front();
        
        // Create OpBuilder at the beginning of the entry block
        OpBuilder builder(&entryBlock, entryBlock.begin());
        
        // Create emitc.VerbatimOp (call init) operation
        builder.create<emitc::VerbatimOp>(funcOp->getLoc(), "init_cryptcontext();");

        return mlir::WalkResult::interrupt();
    });
}