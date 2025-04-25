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
#include "Common/Utils.h"

#define DEBUG_TYPE "insert-emitc-preamble"

using namespace mlir;
using namespace aegis;


constexpr std::string_view kInitCtxFunc = R"cpp(
CryptoContext<DCRTPoly> clientCC;
PublicKey<DCRTPoly> clientPubKey;
extern "C"
bool init_cryptcontext(const std::string &pubKeyLoc, const std::string &multKeyLoc, const std::string &rotKeyLoc) {
    CCParams<CryptoContext{0}RNS> parameters;
    parameters.SetMultiplicativeDepth({1});
    parameters.SetFirstModSize({2});
    parameters.SetScalingModSize({3});
    parameters.SetBatchSize({4});
    clientCC = GenCryptoContext(parameters);
    clientCC->ClearEvalMultKeys();
    clientCC->ClearEvalAutomorphismKeys();
    if (!Serial::DeserializeFromFile(pubKeyLoc, clientPubKey, SerType::BINARY)) {{
        std::cerr << "Cannot read serialized data from: " << pubKeyLoc << std::endl;
        return false;
    }
    std::ifstream multKeyIStream(multKeyLoc, std::ios::in | std::ios::binary);
    if (!multKeyIStream.is_open()) {{
        std::cerr << "Cannot read serialization from " << multKeyLoc << std::endl;
        return false;
    }
    if (!clientCC->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {{
        std::cerr << "Could not deserialize eval mult key file" << std::endl;
        return false;
    }
    if (!rotKeyLoc.empty()) {{
        std::ifstream rotKeyIStream(rotKeyLoc, std::ios::in | std::ios::binary);
        if (!rotKeyIStream.is_open()) {{
            std::cerr << "Cannot read serialization from " << rotKeyLoc << std::endl;
            return false;
        }
        if (!clientCC->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {{
            std::cerr << "Could not deserialize eval rot key file" << std::endl;
            return false;
        }
    }
    return true;
}
)cpp";


constexpr std::string_view kAegisAdaptorFunc = R"cpp(
extern "C" 
std::vector<uint8_t> {0}{1}({2}) {
    {3}
    RLWECipher retV = {4}({5});
    std::stringstream retss;
    Serial::Serialize(retV, retss, SerType::BINARY);
    std::vector<uint8_t> retBuf((std::istreambuf_iterator<char>(retss)), std::istreambuf_iterator<char>());
    return retBuf;
}
)cpp";

constexpr std::string_view kDeserisBufCode = R"cpp(
    Ciphertext<DCRTPoly> v{0};
    std::stringstream ss{1};
    ss{2}.write(reinterpret_cast<const char *>(buf{3}.data()), buf{4}.size());
    Serial::Deserialize(v{5}, ss{6}, SerType::BINARY);
)cpp";


void InsertEmitcPreamblePass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<emitc::EmitCDialect>();
}

void InsertEmitcPreamblePass::runOnOperation() {
    ModuleOp module = getOperation();
    OpBuilder builder(module.getContext());

    // Define the content to be inserted for IncludeOp and VerbatimOp.
    SmallVector<std::pair<StringRef, bool>> incLines = {
        {"vector",    /*isSystem=*/true},
        {"iostream",  /*isSystem=*/true},
        {"openfhe.h", /*isSystem=*/false}
    };

    SmallVector<StringRef> verbatimUsing = {
        "using namespace std;",
        "using namespace lbcrypto;",
        "using CiphertextT = Ciphertext<DCRTPoly>;",
        "using RLWECipher = Ciphertext<DCRTPoly>;",
        "using LWECipher = Ciphertext<DCRTPoly>;",
        "using PlaintextT = Plaintext;",
        "using Plain = double;",
        "using PlainVector = std::vector<double>;",
        "using PlainMatrix = std::vector<PlainVector>;",
        "using MutableCiphertextT = Ciphertext<DCRTPoly>;",
        "using CCParamsT = CCParams<CryptoContextCKKSRNS>;",
        "using CryptoContextT = CryptoContext<DCRTPoly>;",
        "using EvalKeyT = EvalKey<DCRTPoly>;",
        "using PrivateKeyT = PrivateKey<DCRTPoly>;",
        "using PublicKeyT = PublicKey<DCRTPoly>;",
    };

    SmallVector<StringRef> verbatimMacros = {
        "#define Copy(src, dest) dest = src",
        "#define Add(a, b) clientCC->EvalAdd((a), (b))",
        "#define AddPlain(c, p) AddPlainImpl((c), (p))",
        "#define Sub(a, b) clientCC->EvalSub((a), (b))",
        "#define SubPlain(c, p) SubPlainImpl((c), (p))",
        "#define Mul(a, b) clientCC->EvalMult((a), (b))",
        "#define MulPlain(c, p) MulPlainImpl((c), (p))",
        "#define Rotate(c, idx) clientCC->EvalRotate((c), (idx))",
        "#define MakePlain(a)  double(a)",
        "#define MakeMultPlain(...) std::vector<double>{__VA_ARGS__}",
        "#define Cast_Plain_To_Index(clr) size_t(clr)",
        "#define Native_Load(v, idx) v[idx]",
    };

    SmallVector<StringRef> verbatimFuncs = {
        "inline RLWECipher AddPlainImpl(RLWECipher a, Plain b) {",
        "    return clientCC->EvalAdd(a, b);",
        "}",
        "inline RLWECipher AddPlainImpl(RLWECipher a, PlainVector b) {",
        "    return clientCC->EvalAdd(a, clientCC->MakeCKKSPackedPlaintext(b));",
        "}",
        "inline RLWECipher SubPlainImpl(RLWECipher a, Plain b) {",
        "    return clientCC->EvalSub(a, b);",
        "}",
        "inline RLWECipher SubPlainImpl(RLWECipher a, PlainVector b) {",
        "    return clientCC->EvalSub(a, clientCC->MakeCKKSPackedPlaintext(b));",
        "}",
        "inline RLWECipher MulPlainImpl(RLWECipher a, Plain b) {",
        "    return clientCC->EvalMult(a, b);",
        "}",
        "inline RLWECipher MulPlainImpl(RLWECipher a, PlainVector b) {",
        "    return clientCC->EvalMult(a, clientCC->MakeCKKSPackedPlaintext(b));",
        "}",
    };

    // Assemble the implementation body of function aegis_mlir_adapor_%s
    // TODO: We consider the first function we need to adapt and call. In the future, 
    // a unique identifier may need to be added to locate the target function for adaptation and invocation.
    ProgramSpec &progSpec = ProgramSpec::getInstance();
    if (!progSpec.initialize(progSpecFileName)) {
        return;
    }
    ProtoMessage<aegisprotocol::Function> theFunc = progSpec.getFuncInfo()[0];
    std::string mainFuncName = theFunc.asReader().getName();
    std::vector<bool> paramsType;
    for (auto param : theFunc.asReader().getInputs()) {
        if (param.getType()) {
            paramsType.push_back(true);  //cryptext type
        } else {
            paramsType.push_back(false); //plaintext type
        }
    }

    // Assemble params string
    // TODO:If a formal parameter has an clear type annotation, 
    // the arguments passed to it might be of different types.
    std::string allParamsStr;
    for (auto i = 0; i < paramsType.size(); i++) {
        if (i == (paramsType.size()-1)) {
            allParamsStr += ("const std::vector<uint8_t> &buf" + std::to_string(i+1));
        } else {
            allParamsStr += ("const std::vector<uint8_t> &buf" + std::to_string(i+1) + ", ");
        }
    }

    // Assemble the parameter transformation code.
    std::string allTransStr;
    for (auto i = 0; i < paramsType.size(); i++) {
        if (paramsType[i]) {
            auto idx = std::to_string(i+1);
            auto toCipher = std::string(
                    llvm::formatv(kDeserisBufCode.data(), idx, idx, idx, idx, idx, idx, idx));
            allTransStr += toCipher;
        }
    }

    // Assmble the main function all argument string
    std::string allArgumentsStr;
    for (auto i = 0; i < paramsType.size(); i++) {
        if (paramsType[i]) {
            if (i == (paramsType.size()-1)) {
                allArgumentsStr += ("v" + std::to_string(i+1));
            } else {
                allArgumentsStr += ("v" + std::to_string(i+1) + + ", ");
            }
        } else {
            if (i == (paramsType.size()-1)) {
                allArgumentsStr += ("buf" + std::to_string(i+1));
            } else {
                allArgumentsStr += ("buf" + std::to_string(i+1) + + ", ");
            }
        }
    }

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
        // We must dynamically generate the corresponding encryption parameters based on the program.
        int mulDepth = 8;
        int firstModSize = 60;
        int scaleModeSize = 50;
        int batchSize = 4096/2;
        ProtoMessage<aegisprotocol::KeyInfo> keyInfos = progSpec.getKeyInfo();
        mulDepth = keyInfos.asReader().getMultDepth();
        firstModSize = keyInfos.asReader().getFirstModSize();
        scaleModeSize = keyInfos.asReader().getScaleModSize();
        batchSize = keyInfos.asReader().getBatchSize();
        std::string scheme = keyInfos.asReader().getScheme();
        
        auto loadCryptoResFunc = std::string(llvm::formatv(kInitCtxFunc.data(), scheme, std::to_string(mulDepth), 
                                                           std::to_string(firstModSize), std::to_string(scaleModeSize), std::to_string(batchSize)));
        builder.create<emitc::VerbatimOp>(op->getLoc(), loadCryptoResFunc);

        // Insert crypt related implementation functions
        for (auto &stmt : verbatimFuncs) {
            builder.create<emitc::VerbatimOp>(op->getLoc(), stmt);
        }

        // Insert aegis_mlir_xxx implementation functions at the end of the block
        builder.setInsertionPointToEnd(moduleBlock);
        auto adaptorFunc = std::string(
                                llvm::formatv(kAegisAdaptorFunc.data(), EXPORT_FUNCNAME_PRIFIX, mainFuncName,   
                                              allParamsStr, allTransStr, mainFuncName, allArgumentsStr));
        builder.create<emitc::VerbatimOp>(op->getLoc(), adaptorFunc);

        return mlir::WalkResult::interrupt();
    });

    // // Traverse all functions in the module and 
    // // Insert a call to the init_cryptcontext function at the beginning of target function.
    // module.walk([&](func::FuncOp funcOp) {

    //     // Get the entry block of the function
    //     Block &entryBlock = funcOp.getBody().front();
        
    //     // Create OpBuilder at the beginning of the entry block
    //     OpBuilder builder(&entryBlock, entryBlock.begin());
        
    //     // Create emitc.VerbatimOp (call init) operation
    //     builder.create<emitc::VerbatimOp>(funcOp->getLoc(), "init_cryptcontext();");

    //     return mlir::WalkResult::interrupt();
    // });
}
