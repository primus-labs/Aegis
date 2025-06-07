cmake_minimum_required(VERSION 3.16)

project(aegis LANGUAGES CXX C)


find_program(GOLD_LINKER ld.gold)
if(NOT APPLE)
    if(NOT GOLD_LINKER)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            if(EXISTS "/etc/debian_version")
                set(INSTALL_CMD "sudo apt-get install binutils")
            elseif(EXISTS "/etc/redhat-release" OR EXISTS "/etc/centos-release")
                set(INSTALL_CMD "sudo yum install binutils")
            elseif(EXISTS "/etc/arch-release")
                set(INSTALL_CMD "sudo pacman -S binutils")
            else()
                set(INSTALL_CMD "install the 'binutils' package")
            endif()
        else()
            set(INSTALL_CMD "install binutils package for your system")
        endif()

        message(FATAL_ERROR "\n"
            "ld.gold linker not found but is required for this project.\n"
            "Please install the GNU binutils package:\n"
            "  ${INSTALL_CMD}\n")
    else()
        message(STATUS "Found gold linker: ${GOLD_LINKER}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold" CACHE STRING "Use gold linker" FORCE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold" CACHE STRING "Use gold linker" FORCE)
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=gold" CACHE STRING "Use gold linker" FORCE)
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # -fPIC
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(AEGIS_BUILD_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 0)
set(CMAKE_VERBOSE_MAKEFILE OFF)

message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY ===============> ${CMAKE_BINARY_DIR}/bin")
message(STATUS "AEGIS_BUILD_BINARY_DIR         ===============> ${AEGIS_BUILD_BINARY_DIR}")
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
include_directories(SYSTEM ${MLIR_INCLUDE_DIRS})        # MLIR headers
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})        # LLVM headers
include_directories(SYSTEM ${MLIR_TABLEGEN_OUTPUT_DIR}) # MLIR Tabelgen headers
include_directories(${PROJECT_SOURCE_DIR})              # Aegis headers
include_directories(${PROJECT_BINARY_DIR}/include)      # Aegis binary headers 
message(STATUS "MLIR_TABLEGEN_OUTPUT_DIR  ================> ${MLIR_TABLEGEN_OUTPUT_DIR}")
message(STATUS "MLIR_INCLUDE_DIRS   ================> ${MLIR_INCLUDE_DIRS}")
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
#                          Aegis CPU Backend
# *********************************************************************
set(AEGIS_CPU_BACKEND_DIR "${PROJECT_SOURCE_DIR}/../backend/cpu")
set(AEGIS_CPU_FHE_INCLUDE_DIR "${AEGIS_CPU_BACKEND_DIR}/FHE/include")
add_subdirectory(${AEGIS_CPU_BACKEND_DIR} aegis-cpu)
include_directories(${AEGIS_CPU_FHE_INCLUDE_DIR})
message(STATUS "AEGIS_CPU_FHE_INCLUDE_DIR   ================> ${AEGIS_CPU_FHE_INCLUDE_DIR}")


# *********************************************************************
#                          Handle sub-dirs
# *********************************************************************
add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(tools)

set(ENABLE_TESTS_BUILD TRUE)
if(ENABLE_TESTS_BUILD)
  message(STATUS "Enabling Aegis Tests")
  enable_testing()
  add_subdirectory(tests)
endif()
