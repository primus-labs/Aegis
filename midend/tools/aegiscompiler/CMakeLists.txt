get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
#message(STATUS " dialect_libs     =======================>    ${dialect_libs} ")
#message(STATUS " conversion_libs  =======================>    ${conversion_libs} ")
set(DEP_LIBS
        ${dialect_libs}
        ${conversion_libs}
        aegis_cpu
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
        )

add_llvm_executable(aegiscompiler main.cpp)
llvm_update_compile_flags(aegiscompiler)

set(OPENFHE_LIBRARY_PATH ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/lib)
#link_directories(${OPENFHE_LIBRARY_PATH})
target_link_libraries(aegiscompiler PRIVATE ${DEP_LIBS} -L${OPENFHE_LIBRARY_PATH} OPENFHEcore)

add_dependencies(aegiscompiler mlir-headers)

mlir_check_all_link_libraries(aegiscompiler)

