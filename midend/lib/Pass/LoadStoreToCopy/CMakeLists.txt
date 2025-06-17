set(LLVM_TARGET_DEFINITIONS LoadStoreToCopy.td)
mlir_tablegen(LoadStoreToCopy.cpp.inc -gen-rewriters)
add_public_tablegen_target(loadstore2copy)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLoadStoreToCopy
    LoadStoreToCopy.cpp

    DEPENDS
    loadstore2copy
    AegisFHEIncGen
)

target_include_directories(AegisLoadStoreToCopy PUBLIC ${PROJECT_SOURCE_DIR}/include)