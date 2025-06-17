set(LLVM_TARGET_DEFINITIONS AutoBootstrap.td)
mlir_tablegen(AutoBootstrap.cpp.inc -gen-rewriters)
add_public_tablegen_target(autobootstrap)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisAutoBootstrap
    AutoBootstrap.cpp

    DEPENDS
    autobootstrap
    AegisFHEIncGen
)

target_include_directories(AegisAutoBootstrap PUBLIC ${PROJECT_SOURCE_DIR}/include)