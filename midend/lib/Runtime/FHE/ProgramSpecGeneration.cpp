#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>
#include <variant>

#include "capnp/message.h"
#include "protocol.capnp.h"
#include "Runtime/FHE/ProgramSpecGeneration.h"
#include "Common/Protocol.h"
#include "Common/Error.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/abi-breaking.h"
#include "llvm/Support/Error.h"


namespace mlir {
namespace aegis {


llvm::Expected<ProtoMessage<aegisprotocol::ProgSpec>> createProgramSpec(mlir::ModuleOp module) {
    ProtoMessage<aegisprotocol::ProgSpec> progSpes;

    // Get all functions infos from the module.
    auto funcsInfo = getAllFunctionsInfo(module);
    progSpes.asBuilder().setFuncsInfo(funcsInfo.get().asReader());

    // Get fhe key infos from the module.
    auto keyInfo = getKeyInfo(module);
    progSpes.asBuilder().setKeyInfo(keyInfo.get().asReader());

    // Get statistic infos from the module.
    auto statsInfo = getStatsInfo(module);
    progSpes.asBuilder().setStatsInfo(statsInfo.get().asReader());

    return progSpes;
}


llvm::Expected<ProtoMessage<aegisprotocol::Functions>> getAllFunctionsInfo(mlir::ModuleOp module) {
    auto funcs = module.getOps<mlir::func::FuncOp>();
    auto vectFuncsInfo = std::vector<ProtoMessage<aegisprotocol::Function>>();
    for (auto func : funcs) {
        auto unitFuncInfosOrErr = getUnitFunctionInfo(func);
        if (!unitFuncInfosOrErr) {
            return unitFuncInfosOrErr.takeError();
        }
        vectFuncsInfo.push_back(*unitFuncInfosOrErr);
    }

    auto funcsInfo = ProtoMessage<aegisprotocol::Functions>();
    auto funcsBuilder = funcsInfo.asBuilder().initFunctions(vectFuncsInfo.size());
    for (size_t i = 0; i < vectFuncsInfo.size(); i++) {
        funcsBuilder.setWithCaveats(i, vectFuncsInfo[i].asReader());
    }

    return std::move(funcsInfo);
}


llvm::Expected<ProtoMessage<aegisprotocol::Function>> getUnitFunctionInfo(mlir::func::FuncOp funcOp) {
    auto funcType = funcOp.getFunctionType();

    // Retrieve function info
    auto funcInfos = ProtoMessage<aegisprotocol::Function>();
    funcInfos.asBuilder().setName(funcOp.getSymName().str());
    auto inputsBuilder = funcInfos.asBuilder().initInputs(funcType.getNumInputs());

    // deal with inputs
    for (size_t i = 0; i < funcType.getNumInputs(); i++) {
        auto ty = funcType.getInputs()[i];
        auto param = getFuncParamFromType(ty);
        if (!param) {
            return param.takeError();
        }
        inputsBuilder.setWithCaveats(i, param->asReader());
    }

    // deal with outputs
    auto outputsBuilder = funcInfos.asBuilder().initOutputs(funcType.getNumResults());
    for (size_t i = 0; i < funcType.getNumResults(); i++) {
        auto ty = funcType.getResults()[i];
        auto result = getFuncParamFromType(ty);
        if (!result) {
            return result.takeError();
        }
        outputsBuilder.setWithCaveats(i, result->asReader());
    }

    return std::move(funcInfos);
}


llvm::Expected<ProtoMessage<aegisprotocol::FuncParam>> getFuncParamFromType(mlir::Type ty) {
    if (mlir::isa<fhe::LWECipherType>(ty) || mlir::isa<fhe::LWECipherVectorType>(ty) ||
        mlir::isa<fhe::LWECipherMatrixType>(ty) || mlir::isa<fhe::RLWECipherType>(ty) ||
        mlir::isa<fhe::RLWECipherGridType>(ty)) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
        funcParam.asBuilder().setType(true);
        funcParam.asBuilder().getShape().initDimensions(0);
        return std::move(funcParam);
    } else if (mlir::isa<mlir::IntegerType>(ty) || mlir::isa<mlir::FloatType>(ty) ||
               mlir::isa<mlir::IndexType>(ty) ) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
        funcParam.asBuilder().setType(false);
        funcParam.asBuilder().getShape().initDimensions(0);
        return std::move(funcParam);
    } else if (auto tensorTy = mlir::dyn_cast<mlir::RankedTensorType>(ty)) {
        auto funcParam = getFuncParamFromType(tensorTy.getElementType());
        if (!funcParam) {
            return funcParam.takeError();
        }
        auto output = std::move(*funcParam);
        auto shapeBuilder = output.asBuilder().initShape().initDimensions(tensorTy.getRank());
        for (int64_t i = 0; i < tensorTy.getRank(); i++) {
            shapeBuilder.set(i, tensorTy.getShape()[i]);
        }
        return std::move(output);
    }

    return ErrorMsg("Failed to recognize function param for type : ") << ty;
}


llvm::Expected<ProtoMessage<aegisprotocol::KeyInfo>> getKeyInfo(mlir::ModuleOp module) {
    // TODO: We must analyze the specific code to generate the most efficient keyinfo,
    // here we simply set the default value.
    auto keyInfos = ProtoMessage<aegisprotocol::KeyInfo>();
    // keyInfos.asBuilder().setPolyModDegree(8192);
    // auto coffModChBuilder = keyInfos.asBuilder().initCoffModCh(3);
    // coffModChBuilder.set(0, 60);
    // coffModChBuilder.set(1, 60);
    // coffModChBuilder.set(2, 60);
    // keyInfos.asBuilder().setScale(40);
    keyInfos.asBuilder().setMultDepth(8);
    keyInfos.asBuilder().setFirstModSize(60);
    keyInfos.asBuilder().setScaleModSize(50);
    keyInfos.asBuilder().setBatchSize(4096/2); //BatchSize == ringDim / 2, 128bit -> 4096, 192bit -> 8192, 256bit -> 16384
    

    return std::move(keyInfos);
}


llvm::Expected<ProtoMessage<aegisprotocol::StatsInfo>> getStatsInfo(mlir::ModuleOp module) {
    unsigned int mulCnt = 0;
    unsigned int rotateCnt = 0;
    unsigned int cmpCnt = 0;
    unsigned int selectCnt = 0;
    module.walk([&](mlir::func::FuncOp funcOp) {
        funcOp.walk([&](Operation *op) {
        if (mlir::isa<fhe::RLWEMulOp>(op) || mlir::isa<fhe::RLWEMulPlainOp>(op)) {
            mulCnt++;
        } else if (mlir::isa<fhe::RotateOp>(op)) {
            rotateCnt++;
        } else if (mlir::isa<fhe::CmpOp>(op)) {
            cmpCnt++;
        } else if (mlir::isa<fhe::SelectOp>(op)) {
            selectCnt++;
        }
        });
    });

    auto statsInfo = ProtoMessage<aegisprotocol::StatsInfo>();
    statsInfo.asBuilder().setMulCount(mulCnt);
    statsInfo.asBuilder().setRotCount(rotateCnt);
    statsInfo.asBuilder().setBsCount(0);
    statsInfo.asBuilder().setCmpCount(cmpCnt);
    statsInfo.asBuilder().setSelCount(selectCnt);
    statsInfo.asBuilder().setLevel(0);
    return std::move(statsInfo);
}

} // namespace aegis
} // namespace mlir
