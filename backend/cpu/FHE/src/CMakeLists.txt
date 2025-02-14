add_compile_options(-fexceptions)

add_library (
    aegis_cpu
    KeysetGenerator.cpp
)

add_dependencies(aegis_cpu aegis-protocol)

#target_link_libraries(aegis_cpu
#    PUBLIC
#    OPENFHEcore
#)


target_include_directories(aegis_cpu PUBLIC ${AEGIS_CPU_FHE_INCLUDE_DIR})
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/pke/include)
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/binfhe/include)
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/src/core/include)
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/build/src/core)
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/openfhe/third-party/cereal/include)

get_target_property(INCLUDE_DIRS aegis_cpu INCLUDE_DIRECTORIES)
message(STATUS "Include directories for aegis_cpu: ${INCLUDE_DIRS}")

