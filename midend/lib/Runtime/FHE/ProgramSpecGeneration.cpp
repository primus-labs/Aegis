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
    if (!funcsInfo) {
        return funcsInfo.takeError();
    }
    progSpes.asBuilder().setFuncsInfo(funcsInfo.get().asReader());

    // Get fhe key infos from the module.
    auto keyInfo = getKeyInfo(module, options);
    if (!keyInfo) {
        return keyInfo.takeError();
    }
    progSpes.asBuilder().setKeyInfo(keyInfo.get().asReader());

    // Get statistic infos from the module.
    auto statsInfo = getStatsInfo(module);
    if (!statsInfo) {
        return statsInfo.takeError();
    }
    progSpes.asBuilder().setStatsInfo(statsInfo.get().asReader());

    return progSpes;
}


llvm::Expected<ProtoMessage<aegisprotocol::Functions>> getAllFunctionsInfo(mlir::ModuleOp module) {
    auto pubFuncCnt = 0;
    auto funcs = module.getOps<mlir::func::FuncOp>();
    for (auto func : funcs) {
        SymbolTable::Visibility visibility = SymbolTable::getSymbolVisibility(func);
        if (visibility == SymbolTable::Visibility::Public) {
            pubFuncCnt++;
        }
    }
    if (!pubFuncCnt) {
        return ErrorMsg("Module must contain exactly one public function, but found none.");
    }
    if (pubFuncCnt > 1) {
        return ErrorMsg("Module must contain exactly one public function, but found ") << pubFuncCnt;
    }

    auto vectFuncsInfo = std::vector<ProtoMessage<aegisprotocol::Function>>();
    for (auto func : funcs) {
        SymbolTable::Visibility visibility = SymbolTable::getSymbolVisibility(func);
        if (visibility != SymbolTable::Visibility::Public) {
            continue;
        }

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

        // get param name
        llvm::StringRef nameVal;
        if (auto nameAttr = funcOp.getArgAttr(i, ARG_OR_RET_ATTR_NAME)) {
            if (auto strAttr = mlir::dyn_cast<StringAttr>(nameAttr)) {
                nameVal = strAttr.getValue();
            }
        }

        // get param type
        auto ty = funcType.getInputs()[i];
        auto param = getFuncParamFromType(ty, dims, std::string(nameVal));
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

        // get result name
        llvm::StringRef nameVal;
        if (auto nameAttr = funcOp.getResultAttr(i, ARG_OR_RET_ATTR_NAME)) {
            if (auto strAttr = mlir::dyn_cast<StringAttr>(nameAttr)) {
                nameVal = strAttr.getValue();
            }
        }

        // get result type
        auto ty = funcType.getResults()[i];
        auto result = getFuncParamFromType(ty, dims, std::string(nameVal));
        if (!result) {
            return result.takeError();
        }
        outputsBuilder.setWithCaveats(i, result->asReader());
    }

    return std::move(funcInfos);
}


llvm::Expected<ProtoMessage<aegisprotocol::FuncParam>> getFuncParamFromType(mlir::Type ty, const std::vector<int> dims,
                                                                            const std::string& paramName) {
    if (mlir::isa<emitc::OpaqueType>(ty)) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
        funcParam.asBuilder().setName(paramName);
        funcParam.asBuilder().setType(false);
        if (mlir::cast<emitc::OpaqueType>(ty).getValue() == RLWECIPHER_TYPE_NAME ||
            mlir::cast<emitc::OpaqueType>(ty).getValue() == VECT_RLWECIPHER_TYPE_NAME || 
            mlir::cast<emitc::OpaqueType>(ty).getValue() == LWECIPHER_TYPE_NAME||
            mlir::cast<emitc::OpaqueType>(ty).getValue() == VECT_LWECIPHER_TYPE_NAME ||
            mlir::cast<emitc::OpaqueType>(ty).getValue() == MAT_LWECIPHER_TYPE_NAME) {
            funcParam.asBuilder().setType(true);
        }

        // Whether it's ciphertext or plaintext parameters, the parameters need to have dimensions set.
        auto dimensions = funcParam.asBuilder().getShape().initDimensions(dims.size());
        for (size_t i = 0; i < dims.size(); ++i) {
            dimensions.set(i, dims[i]);
        }
        return std::move(funcParam);
    }
    else if (mlir::isa<fhe::LWECipherType>(ty) || mlir::isa<fhe::LWECipherVectorType>(ty) ||
        mlir::isa<fhe::LWECipherMatrixType>(ty) || mlir::isa<fhe::RLWECipherType>(ty) ||
        mlir::isa<fhe::RLWECipherGridType>(ty)) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
        funcParam.asBuilder().setName(paramName);
        funcParam.asBuilder().setType(true);
        auto dimensions = funcParam.asBuilder().getShape().initDimensions(dims.size());
        for (size_t i = 0; i < dims.size(); ++i) {
            dimensions.set(i, dims[i]);
        }
        return std::move(funcParam);
    } else if (mlir::isa<mlir::IntegerType>(ty) || mlir::isa<mlir::FloatType>(ty) ||
               mlir::isa<mlir::IndexType>(ty) ) {
        auto funcParam = ProtoMessage<aegisprotocol::FuncParam>();
        funcParam.asBuilder().setName(paramName);
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
        return getFuncParamFromType(tensorTy.getElementType(), dims, paramName);
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
                // only the last dimension of multi-dimensional parameters represents the packed batch size. 
                if (!arrayAttr.empty()) {
                    mlir::Attribute lastDimAttr = arrayAttr[arrayAttr.size() - 1];
                    if (auto intAttr = mlir::dyn_cast<mlir::IntegerAttr>(lastDimAttr)) {
                        max_size = std::max(max_size, intAttr.getInt());
                    }
                }
            }
        }
    });

    // Adjust batch size to power of 2
    auto batchSize = adjustAndGetBatchSize(max_size);

    // Retrieve the correct multiplication depth value.
    unsigned maxMulDepth = FHE_MAX_MUL_DEPTH_NO_CMP;
    module.walk([&](mlir::Operation *op) {
        if (mlir::isa<emitc::CallOpaqueOp>(op)) {
            if (auto callOp = mlir::dyn_cast_or_null<emitc::CallOpaqueOp>(op)) {
                StringRef calleeName = callOp.getCallee();
                if (calleeName.starts_with(EMITC_CMP_PREFIX_NAME) ||
                    calleeName == EMITC_DIV_NAME || calleeName == EMITC_RECIPROCAL_NAME) {
                    maxMulDepth = FHE_MAX_MUL_DEPTH_WITH_CMP;
                    return mlir::WalkResult::interrupt();
                }
            }
        }
        return mlir::WalkResult::advance();
    });

    // TODO: We must analyze the specific code to generate the most efficient keyinfo,
    auto keyInfos = ProtoMessage<aegisprotocol::KeyInfo>();
    // keyInfos.asBuilder().setPolyModDegree(8192);
    // auto coffModChBuilder = keyInfos.asBuilder().initCoffModCh(3);
    // coffModChBuilder.set(0, 60);
    // coffModChBuilder.set(1, 60);
    // coffModChBuilder.set(2, 60);
    // keyInfos.asBuilder().setScale(40);
    keyInfos.asBuilder().setMultDepth(maxMulDepth);
    keyInfos.asBuilder().setFirstModSize(FHE_FIRST_MOD_SIZE);
    keyInfos.asBuilder().setScaleModSize(FHE_SCALE_MOD_SIZE);
    keyInfos.asBuilder().setBatchSize(batchSize); //BatchSize == ringDim / 2, 128bit -> 4096, 192bit -> 8192, 256bit -> 16384
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
    unsigned int divCnt = 0;
    unsigned int recipCnt = 0;
    unsigned int rotateCnt = 0;
    unsigned int bootstrapCnt = 0;
    unsigned int cmpCnt = 0;
    unsigned int selectCnt = 0;
    
    module.walk([&](mlir::func::FuncOp funcOp) {
        funcOp.walk([&](Operation *op) {
            if (mlir::isa<emitc::CallOpaqueOp>(op)) {
                if (auto callOp = mlir::dyn_cast_or_null<emitc::CallOpaqueOp>(op)) {
                    StringRef calleeName = callOp.getCallee();
                    if (calleeName == EMITC_MUL_NAME || calleeName == EMITC_MULPLAIN_NAME) {
                        mulCnt++;
                    } else if (calleeName == EMITC_DIVPLAIN_NAME) {
                        mulCnt++;   // In fact, the divplain is equivalent to the mulplain.​
                    } else if (calleeName == EMITC_DIV_NAME) {
                        divCnt++;
                    } else if (calleeName == EMITC_RECIPROCAL_NAME) {
                        recipCnt++;
                    } else if (calleeName == EMITC_ROTATE_NAME) {
                        rotateCnt++;
                    } else if (calleeName == EMITC_BOOT_NAME) {
                        bootstrapCnt++;
                    } else if (calleeName == EMITC_SELECT_NAME) {
                        selectCnt++;
                    } else if (calleeName.starts_with(EMITC_CMP_PREFIX_NAME)) {
                        cmpCnt++;
                    }
                }
            }
        });
    });
    
    auto statsInfo = ProtoMessage<aegisprotocol::StatsInfo>();
    statsInfo.asBuilder().setMulCount(mulCnt);
    statsInfo.asBuilder().setDivCount(divCnt);
    statsInfo.asBuilder().setRecipCount(recipCnt);
    statsInfo.asBuilder().setRotCount(rotateCnt);
    statsInfo.asBuilder().setBsCount(bootstrapCnt);
    statsInfo.asBuilder().setCmpCount(cmpCnt);
    statsInfo.asBuilder().setSelCount(selectCnt);
    statsInfo.asBuilder().setLevel(0);
    return std::move(statsInfo);
}

} // namespace aegis
} // namespace mlir
