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
#include "Common/Utils.h"
#include "Dialect/FHE/FHEDialect.h"
#include "Dialect/FHE/FHEOps.h"
#include "Dialect/FHE/FHETypes.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/abi-breaking.h"
#include "llvm/Support/Error.h"


namespace mlir {
namespace aegis {


llvm::Expected<ProtoMessage<aegisprotocol::ProgSpec>> createProgramSpec(mlir::ModuleOp module, CompileOptions options) {
    ProtoMessage<aegisprotocol::ProgSpec> progSpes;

    // Get all functions infos from the module.
    auto funcsInfo = getAllFunctionsInfo(module);
    progSpes.asBuilder().setFuncsInfo(funcsInfo.get().asReader());

    // Get fhe key infos from the module.
    auto keyInfo = getKeyInfo(module, options);
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
        // get param dims
        std::vector<int> dims;
        mlir::DictionaryAttr argAttrs = funcOp.getArgAttrDict(i);
        mlir::Attribute dimsAttr = argAttrs.get(DIMS_ATTR_NAME);
        if (auto arrayAttr = mlir::dyn_cast_or_null<mlir::ArrayAttr>(dimsAttr)) {
            for (mlir::Attribute dimAttr : arrayAttr) {
                if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(dimAttr)) {
                    dims.push_back(intAttr.getInt());
                }
            }
        }

        auto ty = funcType.getInputs()[i];
        auto param = getFuncParamFromType(ty, dims);
        if (!param) {
            return param.takeError();
        }
        inputsBuilder.setWithCaveats(i, param->asReader());
    }

    // deal with outputs
    auto outputsBuilder = funcInfos.asBuilder().initOutputs(funcType.getNumResults());
    for (size_t i = 0; i < funcType.getNumResults(); i++) {
        std::vector<int> dims;
        mlir::DictionaryAttr resAttrs = funcOp.getResultAttrDict(i);
        mlir::Attribute dimsAttr = resAttrs.get(DIMS_ATTR_NAME);
        if (auto arrayAttr = mlir::dyn_cast_or_null<mlir::ArrayAttr>(dimsAttr)) {
            for (mlir::Attribute dimAttr : arrayAttr) {
                if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(dimAttr)) {
                    dims.push_back(intAttr.getInt());
                }
            }
        }

        auto ty = funcType.getResults()[i];
        auto result = getFuncParamFromType(ty, dims);
        if (!result) {
            return result.takeError();
        }
        outputsBuilder.setWithCaveats(i, result->asReader());
    }

    return std::move(funcInfos);
}


llvm::Expected<ProtoMessage<aegisprotocol::FuncParam>> getFuncParamFromType(mlir::Type ty, const std::vector<int> dims) {
    if (mlir::isa<emitc::OpaqueType>(ty)) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
            funcParam.asBuilder().setType(false);
        if (mlir::cast<emitc::OpaqueType>(ty).getValue() == "RLWECipher" ||
            mlir::cast<emitc::OpaqueType>(ty).getValue() == "RLWECipherGrid" || 
            mlir::cast<emitc::OpaqueType>(ty).getValue() == "LWECipher" ||
            mlir::cast<emitc::OpaqueType>(ty).getValue() == "LWECipherVector" ||
            mlir::cast<emitc::OpaqueType>(ty).getValue() == "LWECipherMatrix") {
            funcParam.asBuilder().setType(true);
            auto dimensions = funcParam.asBuilder().getShape().initDimensions(dims.size());
            for (size_t i = 0; i < dims.size(); ++i) {
                dimensions.set(i, dims[i]);
            }
        }
        return std::move(funcParam);
    }
    else if (mlir::isa<fhe::LWECipherType>(ty) || mlir::isa<fhe::LWECipherVectorType>(ty) ||
        mlir::isa<fhe::LWECipherMatrixType>(ty) || mlir::isa<fhe::RLWECipherType>(ty) ||
        mlir::isa<fhe::RLWECipherGridType>(ty)) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
        funcParam.asBuilder().setType(true);
        auto dimensions = funcParam.asBuilder().getShape().initDimensions(dims.size());
        for (size_t i = 0; i < dims.size(); ++i) {
            dimensions.set(i, dims[i]);
        }
        return std::move(funcParam);
    } else if (mlir::isa<mlir::IntegerType>(ty) || mlir::isa<mlir::FloatType>(ty) ||
               mlir::isa<mlir::IndexType>(ty) ) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
        funcParam.asBuilder().setType(false);
        auto dimensions = funcParam.asBuilder().getShape().initDimensions(dims.size());
        for (size_t i = 0; i < dims.size(); ++i) {
            dimensions.set(i, dims[i]);
        }
        return std::move(funcParam);
    } else if (auto tensorTy = mlir::dyn_cast<mlir::RankedTensorType>(ty)) {
        std::vector<int> dims;
        for (int64_t i = 0; i < tensorTy.getRank(); i++) {
            dims.push_back(tensorTy.getShape()[i]);
        }
        return getFuncParamFromType(tensorTy.getElementType(), dims);
    }

    return ErrorMsg("Failed to recognize function param for type : ") << ty;
}


llvm::Expected<ProtoMessage<aegisprotocol::KeyInfo>> getKeyInfo(mlir::ModuleOp module, CompileOptions options) {
    // Get batch size
    int64_t max_size = 1;
    module.walk([&max_size](func::FuncOp funcOp) {
        for (size_t i = 0; i < funcOp.getFunctionType().getNumInputs(); i++) {
            mlir::DictionaryAttr argAttrs = funcOp.getArgAttrDict(i);
            mlir::Attribute dimsAttr = argAttrs.get(DIMS_ATTR_NAME);
            if (auto arrayAttr = mlir::dyn_cast_or_null<mlir::ArrayAttr>(dimsAttr)) {
                for (mlir::Attribute dimAttr : arrayAttr) {
                    if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(dimAttr)) {
                        max_size = std::max(max_size, intAttr.getInt());
                    }
                }
            }
        }
    });

    // TODO: We must analyze the specific code to generate the most efficient keyinfo,
    // here we simply set the default value.
    auto keyInfos = ProtoMessage<aegisprotocol::KeyInfo>();
    // keyInfos.asBuilder().setPolyModDegree(8192);
    // auto coffModChBuilder = keyInfos.asBuilder().initCoffModCh(3);
    // coffModChBuilder.set(0, 60);
    // coffModChBuilder.set(1, 60);
    // coffModChBuilder.set(2, 60);
    // keyInfos.asBuilder().setScale(40);
    keyInfos.asBuilder().setMultDepth(FHE_MAX_MUL_DEPTH);
    keyInfos.asBuilder().setFirstModSize(FHE_FIRST_MOD_SIZE);
    keyInfos.asBuilder().setScaleModSize(FHE_SCALE_MOD_SIZE);
    keyInfos.asBuilder().setBatchSize(max_size); //BatchSize == ringDim / 2, 128bit -> 4096, 192bit -> 8192, 256bit -> 16384
    keyInfos.asBuilder().setEnableBootstrapping(enableFheBoostrapFlag);

    // Set galois key indexs
    llvm::SmallVector<int32_t> galosIndex = mlir::aegis::getAllGaloisIndexs(module);
    auto galoisIndices = keyInfos.asBuilder().initGaloisIndices(galosIndex.size());
    for (size_t i = 0; i < galosIndex.size(); ++i) {
        galoisIndices.set(i, galosIndex[i]);
    }

    // Set fhe scheme
    std::string scheme = options.scheme == FHE_SCHEME_TYPE::CKKS ? "CKKS" : 
                                (options.scheme == FHE_SCHEME_TYPE::BGV ? "BGV" : "BFV");
    keyInfos.asBuilder().setScheme(scheme);

    return std::move(keyInfos);
}


llvm::Expected<ProtoMessage<aegisprotocol::StatsInfo>> getStatsInfo(mlir::ModuleOp module) {
    unsigned int mulCnt = 0;
    unsigned int rotateCnt = 0;
    unsigned int bootstrapCnt = 0;
    unsigned int cmpCnt = 0;
    unsigned int selectCnt = 0;
    // module.walk([&](mlir::func::FuncOp funcOp) {
    //     funcOp.walk([&](Operation *op) {
    //     if (mlir::isa<fhe::RLWEMulOp>(op) || mlir::isa<fhe::RLWEMulPlainOp>(op)) {
    //         mulCnt++;
    //     } else if (mlir::isa<fhe::RotateOp>(op)) {
    //         rotateCnt++;
    //     } else if (mlir::isa<fhe::BootstrapOp>(op)) {
    //         bootstrapCnt++;
    //     } else if (mlir::isa<fhe::CmpOp>(op)) {
    //         cmpCnt++;
    //     } else if (mlir::isa<fhe::SelectOp>(op)) {
    //         selectCnt++;
    //     }
    //     });
    // });
    
    module.walk([&](mlir::func::FuncOp funcOp) {
        funcOp.walk([&](Operation *op) {
            if (mlir::isa<emitc::CallOpaqueOp>(op)) {
                if (auto callOp = mlir::dyn_cast_or_null<emitc::CallOpaqueOp>(op)) {
                    StringRef calleeName = callOp.getCallee();
                    if (calleeName == "Mul" || calleeName == "MulPlain") {
                        mulCnt++;
                    } else if (calleeName == "RotateOp") {
                        rotateCnt++;
                    } else if (calleeName == "Bootstrap") {
                        bootstrapCnt++;
                    } else if (calleeName == "Select") {
                        selectCnt++;
                    } else if (calleeName.starts_with("Cmp_")) {
                        cmpCnt++;
                    }
                }
            }
        });
    });
    
    auto statsInfo = ProtoMessage<aegisprotocol::StatsInfo>();
    statsInfo.asBuilder().setMulCount(mulCnt);
    statsInfo.asBuilder().setRotCount(rotateCnt);
    statsInfo.asBuilder().setBsCount(bootstrapCnt);
    statsInfo.asBuilder().setCmpCount(cmpCnt);
    statsInfo.asBuilder().setSelCount(selectCnt);
    statsInfo.asBuilder().setLevel(0);
    return std::move(statsInfo);
}

} // namespace aegis
} // namespace mlir
