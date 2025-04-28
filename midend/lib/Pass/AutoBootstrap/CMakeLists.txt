
add_mlir_library(
    AegisAutoBootstrap
    AutoBootstrap.cpp
)

target_include_directories(AegisAutoBootstrap PUBLIC ${PROJECT_SOURCE_DIR}/include)