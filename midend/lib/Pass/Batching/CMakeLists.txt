
add_mlir_library(
    AegisBatching
    Batching.cpp
)

target_include_directories(AegisBatching PUBLIC ${PROJECT_SOURCE_DIR}/include)