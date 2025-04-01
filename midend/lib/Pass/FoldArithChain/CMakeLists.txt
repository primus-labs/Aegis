
add_mlir_library(
    AegisFoldArithChain
    FoldArithChain.cpp

    DEPENDS
    AegisFHEIncGen
)

target_include_directories(AegisFoldArithChain PUBLIC ${PROJECT_SOURCE_DIR}/include)