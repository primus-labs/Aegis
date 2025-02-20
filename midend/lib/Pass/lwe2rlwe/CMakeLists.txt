
add_mlir_library(
    AegisLowerLweToRlwe
    LowerLweToRlwe.cpp
)

target_include_directories(AegisLowerLweToRlwe PUBLIC ${PROJECT_SOURCE_DIR}/include)