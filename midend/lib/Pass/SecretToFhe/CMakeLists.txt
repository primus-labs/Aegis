set(LLVM_TARGET_DEFINITIONS LowerSecretToFhe.td)
mlir_tablegen(LowerSecretToFhe.cpp.inc -gen-rewriters)
add_public_tablegen_target(secret2fhe)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerSecretToFhe
    LowerSecretToFhe.cpp

    DEPENDS
    secret2fhe
    AegisSecretIncGen
    AegisFHEIncGen
)

target_include_directories(AegisLowerSecretToFhe PUBLIC ${PROJECT_SOURCE_DIR}/include)
