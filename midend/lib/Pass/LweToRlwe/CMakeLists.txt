
add_mlir_library(
    AegisLowerLweToRlwe
    LowerLweToRlwe.cpp

    DEPENDS
    mlir-headers
    AegisFHEIncGen
)

target_include_directories(AegisLowerLweToRlwe PUBLIC ${PROJECT_SOURCE_DIR}/include)