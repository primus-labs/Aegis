
add_mlir_library(
    AegisMultiDimLoad
    LowerMultiDimLoad.cpp

    DEPENDS
    AegisFHEIncGen
)

target_include_directories(AegisMultiDimLoad PUBLIC ${PROJECT_SOURCE_DIR}/include)