
add_mlir_library(
    AegisInsertEmitcPreamble
    InsertEmitcPreamble.cpp
)

target_include_directories(AegisInsertEmitcPreamble PUBLIC ${PROJECT_SOURCE_DIR}/include)