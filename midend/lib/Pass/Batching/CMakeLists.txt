set(LLVM_TARGET_DEFINITIONS Batching.td)
mlir_tablegen(Batching.cpp.inc -gen-rewriters)
add_public_tablegen_target(batching)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisBatching
    Batching.cpp

    DEPENDS
    batching
    AegisFHEIncGen
)

target_include_directories(AegisBatching PUBLIC ${PROJECT_SOURCE_DIR}/include)