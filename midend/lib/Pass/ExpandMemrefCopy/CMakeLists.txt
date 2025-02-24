add_mlir_library(
    AegisExpandMemrefCopy
    ExpandMemrefCopy.cpp
)

target_include_directories(AegisExpandMemrefCopy PUBLIC ${PROJECT_SOURCE_DIR}/include)