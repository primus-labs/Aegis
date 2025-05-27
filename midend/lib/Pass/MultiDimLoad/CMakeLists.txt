
add_mlir_library(
    AegisLowerMultiDimLoad
    LowerMultiDimLoad.cpp

    DEPENDS
    AegisFHEIncGen
)

target_include_directories(AegisLowerMultiDimLoad PUBLIC ${PROJECT_SOURCE_DIR}/include)