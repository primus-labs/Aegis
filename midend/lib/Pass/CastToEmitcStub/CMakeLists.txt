set(LLVM_TARGET_DEFINITIONS LowerCastToEmitcStub.td)
mlir_tablegen(LowerCastToEmitcStub.cpp.inc -gen-rewriters)
add_public_tablegen_target(cast2emitcstub)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerCastToEmitcStub
    LowerCastToEmitcStub.cpp

    DEPENDS
    cast2emitcstub
    AegisSecretIncGen
    AegisFHEIncGen
)

target_include_directories(AegisLowerCastToEmitcStub PUBLIC ${PROJECT_SOURCE_DIR}/include)