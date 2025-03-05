set(LLVM_TARGET_DEFINITIONS LowerFuncToSecret.td)
mlir_tablegen(LowerFuncToSecret.cpp.inc -gen-rewriters)
add_public_tablegen_target(func2secret)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerFuncToSecret
    LowerFuncToSecret.cpp

    DEPENDS
    mlir-headers
    AegisSecretIncGen
    func2secret
)

target_include_directories(AegisLowerFuncToSecret PUBLIC ${PROJECT_SOURCE_DIR}/include)