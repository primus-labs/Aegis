
message(STATUS "****************************************************")
message(STATUS "PROJECT_SOURCE_DIR ==========> ${PROJECT_SOURCE_DIR}")
message(STATUS "****************************************************")

add_mlir_dialect_library(
    SecretDialect
    SecretDialect.cpp
    SecretOps.cpp

    #ADDITIONAL_HEADER_DIRS
    #${PROJECT_SOURCE_DIR}/include/Dialect/Secret

    DEPENDS
    mlir-headers

    LINK_LIBS
    PUBLIC
    MLIRIR
)


target_include_directories(SecretDialect PUBLIC ${PROJECT_SOURCE_DIR}/include)
get_target_property(INCLUDE_DIRS SecretDialect INCLUDE_DIRECTORIES)
message(STATUS "****************************************************")
message(STATUS "SecretDialect include directories: ${INCLUDE_DIRS}")
message(STATUS "CMAKE_INCLUDE_PATH: ${CMAKE_INCLUDE_PATH}")
message(STATUS "****************************************************")
target_link_libraries(SecretDialect PUBLIC MLIRIR)
