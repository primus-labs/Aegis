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
)

get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
set(RT_DEP_LIBS
    ${dialect_libs}
    ${conversion_libs}
    aegis_cpu
    aegis-protocol 

    MLIREmitCDialect
    MLIROptLib
    MLIRAnalysis
    MLIRCallInterfaces
    MLIRCastInterfaces
    #MLIRExecutionEngine
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

    AegisCommon
    AegisUnrollAndMemOpt
    AegisLowerBranch
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
    AegisLowerMultiDimLoad
    AegisBatching
    AegisLoadStoreToCopy
    AegisLowerLweToRlwe
    AegisAutoBootstrap
    AegisLowerFheToEmitc
    AegisLowerCastToEmitcStub
    AegisInsertEmitcPreamble
    AegisCollectMetadata
    AegisSimRuntime
)

set(OPENFHE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/lib)
target_link_libraries(AegisRuntime PUBLIC ${RT_DEP_LIBS} -L${OPENFHE_LIBRARY_PATH} OPENFHEcore OPENFHEpke OPENFHEbinfhe)
if(APPLE)
target_link_options(AegisRuntime PRIVATE "LINKER:-rpath,@loader_path" "LINKER:-rpath,/usr/local/lib")
endif()
#target_link_options(AegisRuntime PUBLIC "-Wl,--verbose")

target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../backend)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/pke/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/binfhe/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/core/include)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/src/core)
target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/third-party/cereal/include)