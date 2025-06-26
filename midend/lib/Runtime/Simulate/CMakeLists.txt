add_compile_options(-fexceptions -frtti)

add_mlir_library(
    AegisSimRuntime
    SimDataProcessor.cpp
    SimPipeline.cpp
    SimRuntime.cpp
)

target_include_directories(AegisSimRuntime PUBLIC ${PROJECT_SOURCE_DIR}/include)