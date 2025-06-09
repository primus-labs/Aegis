set(LLVM_TARGET_DEFINITIONS LowerFheToEmitc.td)
mlir_tablegen(LowerFheToEmitc.cpp.inc -gen-rewriters)
add_public_tablegen_target(fhe2emitc)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerFheToEmitc
    LowerFheToEmitc.cpp

    DEPENDS
    fhe2emitc
    AegisSecretIncGen
    AegisFHEIncGen
)

target_include_directories(AegisLowerFheToEmitc PUBLIC ${PROJECT_SOURCE_DIR}/include)