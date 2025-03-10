set(LLVM_TARGET_DEFINITIONS LowerMemrefToSecret.td)
mlir_tablegen(LowerMemrefToSecret.cpp.inc -gen-rewriters)
add_public_tablegen_target(memref2secret)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerMemrefToSecret
    LowerMemrefToSecret.cpp

    DEPENDS
    memref2secret
    AegisSecretIncGen
)

target_include_directories(AegisLowerMemrefToSecret PUBLIC ${PROJECT_SOURCE_DIR}/include)