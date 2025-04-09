add_compile_options(-fexceptions -frtti)


add_mlir_library(
    AegisRuntime SHARED
    CompilerEngine.cpp
    FHEDataProcessor.cpp
    FHERuntime.cpp
    FHEPipeline.cpp
    ProgramSpecGeneration.cpp

    DEPENDS
    aegis-protocol

    LINK_LIBS
    PUBLIC
    capnp
    kj
)

get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
set(RT_DEP_LIBS
    ${dialect_libs}
    ${conversion_libs}
    aegis_cpu

    MLIREmitCDialect
    MLIROptLib
    MLIRAnalysis
    MLIRCallInterfaces
    MLIRCastInterfaces
    MLIRExecutionEngine
    MLIRIR
    MLIRLLVMCommonConversion
    MLIRLLVMToLLVMIRTranslation
    MLIRMemRefDialect
    MLIRParser
    MLIRPass
    MLIRInferTypeOpInterface
    MLIRControlFlowInterfaces
    MLIRSideEffectInterfaces
    MLIRTargetLLVMIRExport
    MLIRTransforms

    AegisUnrollAndMemOpt
    AegisUnrollLoops
    AegisGlobalMemrefReplace
    AegisExtractLoopBody
    AegisExpandMemrefCopy
    AegisForwardInsertToExtract
    AegisForwardStoreToLoad
    AegisLowerArithToSecret
    AegisLowerFuncToSecret
    AegisLowerMemrefToSecret
    AegisLowerSecretToFhe
    AegisFoldArithChain
    AegisBatching
    AegisLowerLweToRlwe
    AegisLowerFheToEmitc
    AegisLowerCastToEmitcStub
    AegisInsertEmitcPreamble
    AegisCollectMetadata
    
    AegisCommon
)

set(OPENFHE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/lib)
target_link_libraries(AegisRuntime PRIVATE ${RT_DEP_LIBS} -L${OPENFHE_LIBRARY_PATH} OPENFHEcore)
target_link_options(AegisRuntime PRIVATE -fuse-ld=gold)
#target_link_options(AegisRuntime PRIVATE "-Wl,--verbose")

target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/pke/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/binfhe/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/core/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/src/core)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/third-party/cereal/include)