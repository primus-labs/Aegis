add_mlir_library(
    AegisForwardInsertToExtract
    ForwardInsertToExtract.cpp
)

target_include_directories(AegisForwardInsertToExtract PUBLIC ${PROJECT_SOURCE_DIR}/include)