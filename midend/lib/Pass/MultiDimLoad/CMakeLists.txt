set(LLVM_TARGET_DEFINITIONS LowerMultiDimLoad.td)
mlir_tablegen(LowerMultiDimLoad.cpp.inc -gen-rewriters)
add_public_tablegen_target(multidimload)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerMultiDimLoad
    LowerMultiDimLoad.cpp

    DEPENDS
    multidimload
    AegisFHEIncGen
)

target_include_directories(AegisLowerMultiDimLoad PUBLIC ${PROJECT_SOURCE_DIR}/include)