
add_mlir_library(
    AegisCollectMetadata
    CollectMetadata.cpp
)

target_include_directories(AegisCollectMetadata PUBLIC ${PROJECT_SOURCE_DIR}/include)