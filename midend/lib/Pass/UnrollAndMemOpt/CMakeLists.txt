
add_mlir_library(
    AegisUnrollAndMemOpt
    UnrollLoopAndMemOpt.cpp
)

target_include_directories(AegisUnrollAndMemOpt PUBLIC ${PROJECT_SOURCE_DIR}/include)