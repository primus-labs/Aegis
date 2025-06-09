set(LLVM_TARGET_DEFINITIONS FoldArithChain.td)
mlir_tablegen(FoldArithChain.cpp.inc -gen-rewriters)
add_public_tablegen_target(foldarithchain)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisFoldArithChain
    FoldArithChain.cpp

    DEPENDS
    foldarithchain
    AegisFHEIncGen
)

target_include_directories(AegisFoldArithChain PUBLIC ${PROJECT_SOURCE_DIR}/include)