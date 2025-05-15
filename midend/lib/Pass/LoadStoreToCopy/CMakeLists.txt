

add_mlir_library(
    AegisLoadStoreToCopy
    LoadStoreToCopy.cpp

    DEPENDS
    AegisFHEIncGen
)

target_include_directories(AegisLoadStoreToCopy PUBLIC ${PROJECT_SOURCE_DIR}/include)