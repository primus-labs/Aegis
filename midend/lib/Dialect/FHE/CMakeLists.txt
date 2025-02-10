message(STATUS "****************************************************")
message(STATUS "PROJECT_SOURCE_DIR ==========> ${PROJECT_SOURCE_DIR}")
message(STATUS "****************************************************")

add_mlir_dialect_library(
    FHEDialect
    FHEDialect.cpp
    FHEOps.cpp

    DEPENDS
    mlir-headers
    AegisFHEIncGen

    LINK_LIBS
    PUBLIC
    MLIRIR
)


target_include_directories(FHEDialect PUBLIC ${PROJECT_SOURCE_DIR}/include)
get_target_property(INCLUDE_DIRS FHEDialect INCLUDE_DIRECTORIES)
message(STATUS "****************************************************")
message(STATUS "FHEDialect include directories: ${INCLUDE_DIRS}")
message(STATUS "CMAKE_INCLUDE_PATH: ${CMAKE_INCLUDE_PATH}")
message(STATUS "****************************************************")
target_link_libraries(FHEDialect PUBLIC MLIRIR)