cmake_minimum_required(VERSION 3.16)

project(aegis LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)

message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY ===============> ${CMAKE_BINARY_DIR}/bin")
message(STATUS "PROJECT_SOURCE_DIR             ===============> ${PROJECT_SOURCE_DIR}")

# *********************************************************************
#                          Custom helper functions
# *********************************************************************
include(${PROJECT_SOURCE_DIR}/../cmake/AddAEGIS.cmake)

# Custom CMake Setup (if required)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
message(STATUS "CMAKE_MODULE_PATH              ===============> ${CMAKE_MODULE_PATH}")


# Aegis prefixes all MLIR and LLVM includes with their full path, i.e.,
# "mlir/include/<normal include path>" and "llvm/include/<normal include path>"
# so we need to add these.
foreach(inc_dir IN LISTS MLIR_INCLUDE_DIRS)
  if("${inc_dir}" MATCHES "mlir[/\\]include$")
    get_filename_component(parent_dir "${inc_dir}" DIRECTORY)
    get_filename_component(grandparent_dir "${parent_dir}" DIRECTORY)
    list(APPEND MLIR_INCLUDE_DIRS "${grandparent_dir}")
  endif()
endforeach()

foreach(inc_dir IN LISTS LLVM_INCLUDE_DIRS)
  if("${inc_dir}" MATCHES "llvm[/\\]include$")
    get_filename_component(parent_dir "${inc_dir}" DIRECTORY)
    get_filename_component(grandparent_dir "${parent_dir}" DIRECTORY)
    list(APPEND LLVM_INCLUDE_DIRS "${grandparent_dir}")
  endif()
endforeach()


set(MLIR_TABLEGEN_OUTPUT_DIR ${LLVM_BINARY_DIR}/tools/mlir/include)
message(STATUS "MLIR_TABLEGEN_OUTPUT_DIR       ================> ${MLIR_TABLEGEN_OUTPUT_DIR}")
include_directories(SYSTEM ${MLIR_INCLUDE_DIRS})        # MLIR headers
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})        # LLVM headers
include_directories(SYSTEM ${MLIR_TABLEGEN_OUTPUT_DIR}) # MLIR Tabelgen headers
include_directories(${PROJECT_SOURCE_DIR})              # Aegis headers
include_directories(${PROJECT_BINARY_DIR})              # Aegis headers 
message(STATUS "MLIR_INCLUDE_DIRS   ================> ${MLIR_INCLUDE_DIR}")
message(STATUS "LLVM_INCLUDE_DIRS   ================> ${LLVM_INCLUDE_DIRS}")

# *********************************************************************
#                          Aegis Protocol
# *********************************************************************
set(AEGIS_PROTOCOL_DIR "${PROJECT_SOURCE_DIR}/../tools/aegis-protocol")
message(STATUS "AEGIS_PROTOCOL_DIR             ================> ${AEGIS_PROTOCOL_DIR}")
add_subdirectory(${AEGIS_PROTOCOL_DIR} aegis-protocol)
get_target_property(AEGIS_PROTOCOL_GEN_DIR aegis-protocol BINARY_DIR)
set(AEGIS_PROTOCOL_CAPNP_SRC "${AEGIS_PROTOCOL_GEN_DIR}/capnp_src/c++/src")
include_directories(${AEGIS_PROTOCOL_GEN_DIR})
include_directories(${AEGIS_PROTOCOL_CAPNP_SRC})
get_property(mlir-headers GLOBAL PROPERTY TARGETS)
add_dependencies(mlir-headers aegis-protocol)
install(TARGETS aegis-protocol EXPORT aegis-protocol)
install(EXPORT aegis-protocol DESTINATION "./")


# *********************************************************************
#                          Handle sub-dirs
# *********************************************************************
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(tests)
