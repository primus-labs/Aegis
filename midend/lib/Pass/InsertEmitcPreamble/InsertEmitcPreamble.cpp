#include <tuple>
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
#include "cpu/FHE/include/OpenFheTemplates.h"
#include "Common/ProgramSpec.h"
#include "Common/Protocol.h"
#include "Common/Utils.h"

#define DEBUG_TYPE "insert-emitc-preamble"

using namespace mlir;
using namespace aegis;
using namespace aegiscpu::openfhe;


void InsertEmitcPreamblePass::getDependentDialects(mlir::DialectRegistry &registry) const {
    registry.insert<emitc::EmitCDialect>();
}

void InsertEmitcPreamblePass::runOnOperation() {
    ModuleOp module = getOperation();
    OpBuilder builder(module.getContext());

    // Define the content to be inserted for IncludeOp and VerbatimOp.
    // SmallVector<std::pair<StringRef, bool>> incLines = {
    //     {"vector",    /*isStandard=*/true},
    //     {"iostream",  /*isStandard=*/true},
    //     {"cmath",     /*isStandard=*/true},
    //     {"functional",/*isStandard=*/true},
    //     {"openfhe.h", /*isStandard=*/false},
    // };

    // Assemble the implementation body of function aegis_mlir_adapor_%s
    // TODO: We consider the first function we need to adapt and call. In the future, 
    // a unique identifier may need to be added to locate the target function for adaptation and invocation.
    ProgramSpec &progSpec = ProgramSpec::getInstance();
    if (!progSpec.initialize(progSpecFileName)) {
        return;
    }
    ProtoMessage<aegisprotocol::Function> theFunc = progSpec.getFuncInfo()[0];
    std::string mainFuncName = theFunc.asReader().getName();
    std::vector<std::tuple<bool, int, int>> paramsInfo;
    for (auto param : theFunc.asReader().getInputs()) {
        int inputDims = param.getShape().getDimensions().size() ? 
                        param.getShape().getDimensions().size() : 1;
        int lastDim = param.getShape().getDimensions()[inputDims - 1];

        if (param.getType()) {
            paramsInfo.push_back({true, inputDims, lastDim});  //cryptext type
        } else {
            paramsInfo.push_back({false, inputDims, lastDim}); //plaintext type
        }
    }

    // Assemble params string
    // TODO:If a formal parameter has an clear type annotation, 
    // the arguments passed to it might be of different types.
    std::string allParamsStr;
    for (auto i = 0; i < paramsInfo.size(); i++) {
        if (std::get<1>(paramsInfo[i]) > 1) {
            if (i == (paramsInfo.size()-1)) {
                allParamsStr += ("const std::vector<std::vector<uint8_t>> &buf" + std::to_string(i+1));
            } else {
                allParamsStr += ("const std::vector<std::vector<uint8_t>> &buf" + std::to_string(i+1) + ", ");
            }
        } else {
            if (i == (paramsInfo.size()-1)) {
                allParamsStr += ("const std::vector<uint8_t> &buf" + std::to_string(i+1));
            } else {
                allParamsStr += ("const std::vector<uint8_t> &buf" + std::to_string(i+1) + ", ");
            }
        }
    }

    // Assemble the parameter transformation code.
    std::string allTransStr;
    for (auto i = 0; i < paramsInfo.size(); i++) {
        if (std::get<0>(paramsInfo[i])) {
            if (std::get<1>(paramsInfo[i]) > 1) {
                auto idx = std::to_string(i+1);
                auto toCipher = std::string(llvm::formatv(kDeserisBufToMultiCipher.data(), idx));
                allTransStr += toCipher;
            } else {
                auto idx = std::to_string(i+1);
                auto toCipher = std::string(llvm::formatv(kDeserisBufToSingleCipher.data(), idx));
                allTransStr += toCipher;
            }
        } else {
            if (std::get<1>(paramsInfo[i]) > 1) {
                auto idx = std::to_string(i+1);
                auto toMatDouble = std::string(llvm::formatv(kDeserisBufToMatDouble.data(), idx));
                allTransStr += toMatDouble;
            } else {
                if (std::get<2>(paramsInfo[i]) == 1) {
                    auto idx = std::to_string(i+1);
                    auto toSingleDouble = std::string(llvm::formatv(kDeserisBufToSingleDouble.data(), idx));
                    allTransStr += toSingleDouble;
                } else {
                    auto idx = std::to_string(i+1);
                    auto toVectDouble = std::string(llvm::formatv(kDeserisBufToVectDouble.data(), idx));
                    allTransStr += toVectDouble;
                }
            }
        }
    }

    // Assmble the main function all argument string
    std::string allArgumentsStr;
    for (auto i = 0; i < paramsInfo.size(); i++) {
        if (i == (paramsInfo.size()-1)) {
            allArgumentsStr += ("v" + std::to_string(i+1));
        } else {
            allArgumentsStr += ("v" + std::to_string(i+1) + ", ");
        }
    }

    // Traverse all ModuleOp instances and insert emitc::IncludeOp and emitc::VerbatimOp before each of them.
    // Terminate traversal after finding the first one.
    module.walk([&](mlir::ModuleOp op) {
        // Ensure the insertion point is in the top-level block of the module.
        auto moduleBlock = op.getBody()->front().getBlock();
        builder.setInsertionPointToStart(moduleBlock);

        // Insert all emitc::IncludeOp
        // for (auto &include : incLines) {
        //     builder.create<emitc::IncludeOp>(op->getLoc(), include.first, include.second);
        // }
        builder.create<emitc::VerbatimOp>(op->getLoc(), kIncludeStmts);

        // Insert all using stmts
        builder.create<emitc::VerbatimOp>(op->getLoc(), kUsingStmts);

        // Insert all macros
        builder.create<emitc::VerbatimOp>(op->getLoc(), kMacroStmts);

        // Insert loadCryptoResources function
        builder.create<emitc::VerbatimOp>(op->getLoc(), kLoadCryptoResFunc);

        // Insert init cryptcontext function
        // We must dynamically generate the corresponding encryption parameters based on the program.
        ProtoMessage<aegisprotocol::KeyInfo> keyInfos = progSpec.getKeyInfo();
        int mulDepth = keyInfos.asReader().getMultDepth();
        int firstModSize = keyInfos.asReader().getFirstModSize();
        int scaleModeSize = keyInfos.asReader().getScaleModSize();
        int batchSize = keyInfos.asReader().getBatchSize();
        std::string scheme = keyInfos.asReader().getScheme();
        bool enableBootstrap = keyInfos.asReader().getEnableBootstrapping();
        int outputDimSize = progSpec.getFuncInfo()[0].asReader().getOutputs()[0].getShape().getDimensions().size();
        
        auto loadCryptoResFunc = std::string(llvm::formatv(kInitCtxFunc.data(), std::to_string(mulDepth), std::to_string(enableBootstrap), scheme, 
                                                           std::to_string(firstModSize), std::to_string(scaleModeSize), std::to_string(batchSize)));
        builder.create<emitc::VerbatimOp>(op->getLoc(), loadCryptoResFunc);

        // Insert crypto related implementation functions
        builder.create<emitc::VerbatimOp>(op->getLoc(), kFheUtilFuncs);

        // Insert compare related implementation functions
        ProtoMessage<aegisprotocol::StatsInfo> statsInfos = progSpec.getStatsInfo();
        if (statsInfos.asReader().getCmpCount() || statsInfos.asReader().getSelCount()) {
            auto cmpFuncs = std::string(llvm::formatv(kCmpFuncsTemplate.data(), batchSize));
            builder.create<emitc::VerbatimOp>(op->getLoc(), cmpFuncs);
        }

        // Insert alloc implementation functions
        auto allocFunc = std::string(llvm::formatv(kAllocFunc.data(), batchSize));
        builder.create<emitc::VerbatimOp>(op->getLoc(), allocFunc);

        // Insert aegis_mlir_xxx implementation functions at the end of the block
        builder.setInsertionPointToEnd(moduleBlock);
        std::string callFuncStr;
        if (outputDimSize > 1) {
            callFuncStr = mainFuncName + "(" + allArgumentsStr + ")";
        } else {
            callFuncStr = "1, " + mainFuncName + "(" + allArgumentsStr + ")";
        }
        auto adaptorFunc = std::string(
                                llvm::formatv(kAegisAdaptorFunc.data(), EXPORT_FUNCNAME_PRIFIX, mainFuncName,   
                                              allParamsStr, allTransStr, callFuncStr));
        builder.create<emitc::VerbatimOp>(op->getLoc(), adaptorFunc);

        return mlir::WalkResult::interrupt();
    });

    // // Traverse all functions in the module and 
    // // Insert a call to the initCryptContext function at the beginning of target function.
    // module.walk([&](func::FuncOp funcOp) {

    //     // Get the entry block of the function
    //     Block &entryBlock = funcOp.getBody().front();
        
    //     // Create OpBuilder at the beginning of the entry block
    //     OpBuilder builder(&entryBlock, entryBlock.begin());
        
    //     // Create emitc.VerbatimOp (call init) operation
    //     builder.create<emitc::VerbatimOp>(funcOp->getLoc(), "initCryptContext();");

    //     return mlir::WalkResult::interrupt();
    // });
}
