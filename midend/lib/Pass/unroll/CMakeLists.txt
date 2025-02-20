
add_mlir_library(
    AegisUnrollLoops
    UnrollLoops.cpp
)

target_include_directories(AegisUnrollLoops PUBLIC ${PROJECT_SOURCE_DIR}/include)