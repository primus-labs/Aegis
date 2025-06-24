cmake_minimum_required(VERSION 3.14)
project(primus_aegis)
set(target_name primus_aegis)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.11.1
)
message(STATUS "Starting pybind11 download...")
FetchContent_MakeAvailable(pybind11)
message(STATUS "pybind11 downloaded successfully.")
# find_package(pybind11 REQUIRED)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
message(STATUS "Starting nlohmann_json download...")
FetchContent_MakeAvailable(nlohmann_json)
message(STATUS "pybind11 nlohmann_json successfully.")

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python_INCLUDE_DIRS})

# The following two forms of library files can be imported directly into python by `import primus_aegis`
IF(TRUE)
  # generate such as "primus_aegis.cpython-version-arch-xxx.so"
  pybind11_add_module(primus_aegis primus_aegis.cpp)
  set_source_files_properties(primus_aegis.cpp PROPERTIES COMPILE_FLAGS "-fvisibility=default -fno-lto -ffat-lto-objects")
ELSE()
  # generate "primus_aegis.so", more general
  add_library(primus_aegis SHARED primus_aegis.cpp)
  set_target_properties(primus_aegis PROPERTIES PREFIX "")
ENDIF()
target_link_libraries(${target_name} PRIVATE pybind11::headers pybind11::module nlohmann_json::nlohmann_json)
target_link_libraries(${target_name} PRIVATE Python3::Python)

target_link_libraries(${target_name}
  PRIVATE
  aegis_cpu
  AegisCommon
  AegisRuntime
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
