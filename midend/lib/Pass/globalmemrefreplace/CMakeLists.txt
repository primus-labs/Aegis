add_mlir_library(
    AegisGlobalMemrefReplace
    GlobalMemrefReplace.cpp
)

target_include_directories(AegisGlobalMemrefReplace PUBLIC ${PROJECT_SOURCE_DIR}/include)