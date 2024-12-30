add_compile_options(-fexceptions -fno-rtti)

add_library (
    aegis_cpu
    FheKeyset.cpp
    KeysetBuilder.cpp
)

target_link_libraries(aegis_cpu
#    PUBLIC
#    openfhe
)


target_include_directories(aegis_cpu PUBLIC ${AEGIS_CPU_FHE_INCLUDE_DIR})
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/../../third_party/openfhe/src/pke)
target_include_directories(aegis_cpu PUBLIC ${PROJECT_SOURCE_DIR}/../../third_party/openfhe/src/binfhe)

#get_target_property(INCLUDE_DIRS aegis_cpu INCLUDE_DIRECTORIES)
#message(STATUS "Include directories for aegis_cpu: ${INCLUDE_DIRS}")
