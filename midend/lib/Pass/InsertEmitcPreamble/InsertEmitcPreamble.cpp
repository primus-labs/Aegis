#include "mlir/include/mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/Conversion/MemRefToEmitC/MemRefToEmitC.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/include/llvm/Support/Debug.h"
#include "llvm/Support/FormatVariadic.h"
#include "mlir/include/mlir/Support/LLVM.h" 
#include "mlir/include/mlir/IR/MLIRContext.h"            
#include "mlir/include/mlir/IR/PatternMatch.h"           
#include "Pass/InsertEmitcPreamble/InsertEmitcPreamble.h"
#include "Common/ProgramSpec.h"
#include "Common/Protocol.h"

#define DEBUG_TYPE "insert-emitc-preamble"

using namespace mlir;
using namespace aegis;

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
        "using Plain = Plaintext;",
        "using MutableCiphertextT = Ciphertext<DCRTPoly>;",
        "using CCParamsT = CCParams<CryptoContextCKKSRNS>;",
        "using CryptoContextT = CryptoContext<DCRTPoly>;",
        "using EvalKeyT = EvalKey<DCRTPoly>;",
        "using PrivateKeyT = PrivateKey<DCRTPoly>;",
        "using PublicKeyT = PublicKey<DCRTPoly>;",
    };

    SmallVector<StringRef> verbatimMacros = {
        "#define Add(a, b) cryptoCtx->EvalAdd((a), (b))",
        "#define AddPlain(c, p) cryptoCtx->EvalAdd((c), (p))",
        "#define Sub(a, b) cryptoCtx->EvalSub((a), (b))",
        "#define SubPlain(c, p) cryptoCtx->EvalSub((c), (p))",
        "#define Mul(a, b) cryptoCtx->EvalMult((a), (b))",
        "#define MulPlain(c, p) cryptoCtx->EvalMult((c), (p))",
        "#define Rotate(c, idx) cryptoCtx->EvalRotate((c), (idx))",
        "#define MakePlain(...)  cryptoCtx->MakeCKKSPackedPlaintext(std::vector<double>{__VA_ARGS__})",
        "#define Cast_Plain_To_Index(pt) pt->GetRealPackedValue()[0]",
        "#define Native_Load(v, idx) v[idx]",
    };

    // We must dynamically generate the corresponding encryption parameters based on the program.
    int mulDepth = 8;
    int firstModSize = 60;
    int scaleModeSize = 50;
    int batchSize = 4096/2;
    ProgramSpec &progSpecObj = ProgramSpec::getInstance();
    if (progSpecObj.initialize(progSpecFileName)) {
        ProtoMessage<aegisprotocol::KeyInfo> keyInfos = progSpecObj.getKeyInfo();
        mulDepth = keyInfos.asBuilder().getMultDepth();
        firstModSize = keyInfos.asBuilder().getFirstModSize();
        scaleModeSize = keyInfos.asBuilder().getScaleModSize();
        batchSize = keyInfos.asBuilder().getBatchSize();
    }

    SmallVector<StringRef> verbatimInitCC;
    std::string lineMulDepth     = std::string(llvm::formatv("   parameters.SetMultiplicativeDepth({0});", mulDepth));
    std::string lineFirstModSize = std::string(llvm::formatv("   parameters.SetFirstModSize({0});", firstModSize));
    std::string lineScaleModSize = std::string(llvm::formatv("   parameters.SetScalingModSize({0});", scaleModeSize));
    std::string lineBatchSize    = std::string(llvm::formatv("   parameters.SetBatchSize({0});", batchSize));
    verbatimInitCC.push_back("CryptoContext<DCRTPoly> cryptoCtx;");
    verbatimInitCC.push_back("void init_cryptcontext() {");
    verbatimInitCC.push_back("   CCParams<CryptoContextCKKSRNS> parameters;"); 
    verbatimInitCC.push_back(lineMulDepth);
    verbatimInitCC.push_back(lineFirstModSize);
    verbatimInitCC.push_back(lineScaleModSize);
    verbatimInitCC.push_back(lineBatchSize);
    verbatimInitCC.push_back("   cryptoCtx = GenCryptoContext(parameters);");
    verbatimInitCC.push_back("   cryptoCtx->Enable(PKE);");
    verbatimInitCC.push_back("   cryptoCtx->Enable(KEYSWITCH);");
    verbatimInitCC.push_back("   cryptoCtx->Enable(LEVELEDSHE);");

    // SmallVector<StringRef> verbatimInitCC = {
    //     "CryptoContext<DCRTPoly> cryptoCtx;",
    //     "void init_cryptcontext() {",
    //     "   CCParams<CryptoContextCKKSRNS> parameters;",
    //     "   parameters.SetMultiplicativeDepth(8);",
    //     "   parameters.SetFirstModSize(60);",
    //     "   parameters.SetScalingModSize(50);",
    //     "   parameters.SetBatchSize(4096/2);",
    //     "   cryptoCtx = GenCryptoContext(parameters);",
    //     "   cryptoCtx->Enable(PKE);",
    //     "   cryptoCtx->Enable(KEYSWITCH);",
    //     "   cryptoCtx->Enable(LEVELEDSHE);",
    //     "}",
    // };

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
