add_compile_options(-fexceptions -fno-rtti)

add_mlir_library(
    AegisCommon
    Value.cpp
    Utils.cpp

    DEPENDS
    aegis-protocol

    LINK_LIBS
    PUBLIC
    capnp
    kj
)

target_include_directories(AegisCommon PUBLIC ${PROJECT_SOURCE_DIR}/include)