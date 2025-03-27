cmake_minimum_required(VERSION 3.14)
project(aegis_c)
set(target_name aegis_c)

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python_INCLUDE_DIRS})

# The following two forms of library files can be imported directly into python by `import aegis_c`
IF(TRUE)
  # generate such as "aegis_c.cpython-version-arch-xxx.so"
  pybind11_add_module(aegis_c aegis_c.cpp)
ELSE()
  # generate "aegis_c.so", more general
  add_library(aegis_c SHARED aegis_c.cpp)
  set_target_properties(aegis_c PROPERTIES PREFIX "")
ENDIF()
target_link_libraries(${target_name} PRIVATE pybind11::headers)

target_link_libraries(${target_name}
  PRIVATE
  aegis_cpu
  AegisCommon
  OPENFHEcore
  OPENFHEpke
)

target_include_directories(${target_name} PRIVATE ${AEGIS_CPU_FHE_INCLUDE_DIR})
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/../third_party/openfhe/src/pke/include)
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/../third_party/openfhe/src/binfhe/include)
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/../third_party/openfhe/src/core/include)
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/../third_party/openfhe/build/src/core)
target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/../third_party/openfhe/third-party/cereal/include)

target_link_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/../third_party/openfhe/build/lib)
