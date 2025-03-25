add_mlir_library(
    AegisInsertRelinearize
    InsertRelinearize.cpp
)

target_include_directories(AegisInsertRelinearize PUBLIC ${PROJECT_SOURCE_DIR}/include)