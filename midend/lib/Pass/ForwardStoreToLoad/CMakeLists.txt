add_mlir_library(
    AegisForwardStoreToLoad
    ForwardStoreToLoad.cpp
)

target_include_directories(AegisForwardStoreToLoad PUBLIC ${PROJECT_SOURCE_DIR}/include)