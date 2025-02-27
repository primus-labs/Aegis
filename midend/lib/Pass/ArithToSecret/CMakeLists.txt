set(LLVM_TARGET_DEFINITIONS LowerArithToSecret.td)
mlir_tablegen(LowerArithToSecret.cpp.inc -gen-rewriters)
add_public_tablegen_target(arith2secret)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerArithToSecret
    LowerArithToSecret.cpp

    DEPENDS
    mlir-headers
    AegisSecretIncGen
    arith2secret
)

target_include_directories(AegisLowerArithToSecret PUBLIC ${PROJECT_SOURCE_DIR}/include)
