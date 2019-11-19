cmake_minimum_required(VERSION 2.8.12)
project(Csound-plugins)
set(APIVERSION "6.0")

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(TestBigEndian)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

### COMPILER OPTIMIZATION FLAGS
option(USE_COMPILER_OPTIMIZATIONS "Use the default Csound compiler optimization flags" ON)
if(USE_COMPILER_OPTIMIZATIONS)
include(${CMAKE_SOURCE_DIR}/cmake/CompilerOptimizations.cmake)
endif()

if(APPLE)
    set(OSX_VERSION " ")
endif()

## USER OPTIONS ##
# Optional targets, they should all default to ON (check_deps will disable them if not possible to build)
option(USE_DOUBLE "Set to use double-precision floating point for audio samples." ON)
option(USE_LRINT "Use lrint/lrintf for converting floating point values to integers." ON)

find_package(Csound)

# Python Opcodes
add_subdirectory(Py)
