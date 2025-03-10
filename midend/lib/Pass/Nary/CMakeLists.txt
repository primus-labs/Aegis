
add_mlir_library(
    AegisNary
    Nary.cpp

    DEPENDS
    AegisFHEIncGen
)

target_include_directories(AegisNary PUBLIC ${PROJECT_SOURCE_DIR}/include)