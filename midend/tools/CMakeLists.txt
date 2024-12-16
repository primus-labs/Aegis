get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
message(STATUS " dialect_libs     =======================>    ${dialect_libs} ")
message(STATUS " conversion_libs  =======================>    ${conversion_libs} ")
set(DEP_LIBS
        ${dialect_libs}
        ${conversion_libs}
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
target_link_libraries(aegiscompiler PRIVATE ${LIBS})

add_dependencies(aegiscompiler mlir-headers)

mlir_check_all_link_libraries(aegiscompiler)

