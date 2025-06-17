set(LLVM_TARGET_DEFINITIONS InsertRelinearize.td)
mlir_tablegen(InsertRelinearize.cpp.inc -gen-rewriters)
add_public_tablegen_target(insertrelinearize)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisInsertRelinearize
    InsertRelinearize.cpp

    DEPENDS
    insertrelinearize
    AegisFHEIncGen
)

target_include_directories(AegisInsertRelinearize PUBLIC ${PROJECT_SOURCE_DIR}/include)