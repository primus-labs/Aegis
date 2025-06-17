set(LLVM_TARGET_DEFINITIONS LowerBranch.td)
mlir_tablegen(LowerBranch.cpp.inc -gen-rewriters)
add_public_tablegen_target(branch)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_mlir_library(
    AegisLowerBranch
    LowerBranch.cpp

    DEPENDS
    branch
    AegisFHEIncGen
)

target_include_directories(AegisLowerBranch PUBLIC ${PROJECT_SOURCE_DIR}/include)