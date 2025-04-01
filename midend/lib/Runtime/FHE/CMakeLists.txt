add_compile_options(-fexceptions)

add_mlir_library(
    AegisRuntime
    CompilerEngine.cpp
    FHEDataProcessor.cpp
    FHERuntime.cpp
    FHEPipeline.cpp

    DEPENDS
    aegis-protocol

    LINK_LIBS
    PUBLIC
    capnp
    kj
)

target_include_directories(AegisRuntime PUBLIC ${PROJECT_SOURCE_DIR}/include)