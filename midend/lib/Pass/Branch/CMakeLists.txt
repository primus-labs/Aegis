
add_mlir_library(
    AegisLowerBranch
    LowerBranch.cpp
)

target_include_directories(AegisLowerBranch PUBLIC ${PROJECT_SOURCE_DIR}/include)