set(LLVM_TARGET_DEFINITIONS LowerLweToRlwe.td)
mlir_tablegen(LowerLweToRlwe.cpp.inc -gen-rewriters)
add_public_tablegen_target(lwe2rlwe)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerLweToRlwe
    LowerLweToRlwe.cpp

    DEPENDS
    lwe2rlwe
    AegisFHEIncGen
)

target_include_directories(AegisLowerLweToRlwe PUBLIC ${PROJECT_SOURCE_DIR}/include)