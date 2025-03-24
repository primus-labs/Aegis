add_mlir_library(
    AegisExtractLoopBody
    ExtractLoopBody.cpp
)

target_include_directories(AegisExtractLoopBody PUBLIC ${PROJECT_SOURCE_DIR}/include)