cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

option(USE_VCPKG "Use VCPKG to download and manage dependencies" OFF)

if(USE_VCPKG)
    # Detect correct triplet based on CMake env
    if (WIN32)
        set(VCPKG_HOST_TRIPLET "x64-windows-static-md" CACHE STRING "")
        set(VCPKG_TARGET_TRIPLET "x64-windows-static-md")
    elseif(APPLE)
        set(VCPKG_HOST_TRIPLET "x64-osx" CACHE STRING "")
        set(VCPKG_TARGET_TRIPLET "x64-osx")
    elseif (UNIX)
        set(VCPKG_HOST_TRIPLET "x64-linux" CACHE STRING "")
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()

    set(VCPKG_FEATURE_FLAGS versions)
    set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}")
    set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_SOURCE_DIR}")
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(Csound-plugins)

set(APIVERSION "6.0")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(TestBigEndian)
include(CheckFunctionExists)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

### COMPILER OPTIMIZATION FLAGS
option(USE_COMPILER_OPTIMIZATIONS "Use the default Csound compiler optimization flags" ON)
if(USE_COMPILER_OPTIMIZATIONS)
    include("${CMAKE_SOURCE_DIR}/cmake/CompilerOptimizations.cmake")
endif()

##/Library/Frameworks/CsoundLib64.framework/Versions/Current/Resources/Opcodes64

if(APPLE)
    message(STATUS "Building on OSX")
    set(OSX_VERSION " ")
endif()

## USER OPTIONS ##
# Optional targets, they should all default to ON (check_deps will disable them if not possible to build)
option(USE_DOUBLE "Set to use double-precision floating point for audio samples." ON)
option(USE_LRINT "Use lrint/lrintf for converting floating point values to integers." ON)
option(BUILD_RELEASE "Build for release" ON)
option(USE_GIT_COMMIT "Show the git commit in version information" ON)
option(REQUIRE_PTHREADS "For non-Windows systems, set whether Csound will use threads or not" ON)

# in Release configuration, set NDEBUG
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message("-----> Release mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    message("-----> Debug mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBETA")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBETA")
endif()

# set -Werror if in Debug configuration
if(NOT MSVC AND NOT WASM)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 ")
    set(CMAKE_C_FLAGS_RELEASE "-O3 ")
    if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -Werror -Wno-missing-field-initializers")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -Wno-missing-field-initializers")
    endif()
endif()

if(NOT MSVC AND REQUIRE_PTHREADS)
  # Use ladder of if's to support older CMake versions (i.e., travis)
  find_library(PTHREAD_LIBRARY winpthread-1)

  if(NOT PTHREAD_LIBRARY)
    find_library(PTHREAD_LIBRARY pthread)
  endif()

  if(NOT PTHREAD_LIBRARY)
    find_library(PTHREAD_LIBRARY pthreadGC2)
  endif()

  if(NOT (PTHREAD_LIBRARY OR HAIKU))
      # pthreads are built in to Haiku
      message(STATUS "Csound requires the pthread library")
  endif()

  #FIXME this variable is for required include directories, possible bug
  list(APPEND CMAKE_REQUIRED_INCLUDES pthread.h)
  if (NOT HAIKU)
    set(CMAKE_REQUIRED_LIBRARIES pthread)
  endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LINUX YES)
else()
    set(LINUX NO)
endif()

set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
check_c_compiler_flag(-fvisibility=hidden HAS_VISIBILITY_HIDDEN)
check_cxx_compiler_flag(-fvisibility=hidden HAS_CXX_VISIBILITY_HIDDEN)

if (HAS_VISIBILITY_HIDDEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
endif()

if (HAS_CXX_VISIBILITY_HIDDEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

check_c_compiler_flag(-std=gnu99 HAS_GNU99)
if (HAS_GNU99)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
endif()

if (HAS_CXX_VISIBILITY_HIDDEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

if(LINUX)
    option(USE_LIB64 "Set to on to set installation directory for libraries to lib64" OFF)

    if(USE_LIB64)
        set(LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib64")
        add_definitions("-DLIB64")
    else()
        set(LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
    endif()

    message(STATUS "LIBRARY INSTALL DIR: ${LIBRARY_INSTALL_DIR}")
endif()

# plugin install directories (CS_USER_PLUGIN on MacOs/Windows)
if(USE_DOUBLE)
    message(STATUS "Building with 64-bit floats")
    if(APPLE)
        set(PLUGIN_INSTALL_DIR "$ENV{HOME}/Library/csound/${APIVERSION}/plugins64")
    elseif(LINUX)
        set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins64-${APIVERSION}")
    elseif(MSVC)
        set(PLUGIN_INSTALL_DIR "$ENV{LOCALAPPDATA}/csound/${APIVERSION}/plugins64")
    endif()
else()
    message(STATUS "Building with 32-bit floats")
    if(APPLE)
        set(PLUGIN_INSTALL_DIR "$ENV{HOME}/Library/csound/${APIVERSION}/plugins")
    elseif(LINUX)
        set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins-${APIVERSION}")
    elseif(MSVC)
        set(PLUGIN_INSTALL_DIR "$ENV{LOCALAPPDATA}/csound/${APIVERSION}/plugins")
    endif()
endif()

message("-- Csound headers: ${CSOUND_INCLUDE_DIR}")
if(APPLE)
   message("-- Csound framework: ${CSOUND_FRAMEWORK}")
endif()
message("-- Plugins to be installed at: ${PLUGIN_INSTALL_DIR}")

# Include utility functions for making plugins
include(cmake/utilities.cmake)

# Checking pthread functions
if(REQUIRE_PTHREADS AND (PTHREAD_LIBRARY OR HAIKU))
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_PTHREAD")

  check_function_exists(pthread_spin_lock PTHREAD_SPIN_LOCK_EXISTS)
  check_function_exists(pthread_barrier_init PTHREAD_BARRIER_INIT_EXISTS)

  if(PTHREAD_SPIN_LOCK_EXISTS)
     list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_SPIN_LOCK)
  endif()

  if(PTHREAD_BARRIER_INIT_EXISTS)
     list(APPEND libcsound_CFLAGS -DHAVE_PTHREAD_BARRIER_INIT)
  endif()
endif()

# Linux does not have a separate libintl, it is part of libc
set(LIBINTL_AVAIL (LIBINTL_LIBRARY OR LINUX))

if(LINUX)
    message(STATUS "Building on Linux.")
    add_definitions(-DLINUX -DPIPES -DNO_FLTK_THREADS -D_GNU_SOURCE -DHAVE_SOCKETS)
    list(APPEND libcsound_LIBS ${MATH_LIBRARY} dl)

    find_library(LIBRT_LIBRARY rt)

    if(LIBRT_LIBRARY)
      list(APPEND libcsound_LIBS ${LIBRT_LIBRARY})
      message(STATUS "  ADDING LIBRT LIBRARY: ${LIBRT_LIBRARY}.")
    endif()

    find_library(LIBEXECINFO_LIBRARY execinfo)

    if(LIBEXECINFO_LIBRARY)
       list(APPEND libcsound_LIBS ${LIBEXECINFO_LIBRARY})
       message(STATUS "  ADDING LIBEXECINFO LIBRARY: ${LIBEXECINFO_LIBRARY}.")
    endif()
endif()

if(APPLE AND NOT IOS)
    add_definitions(-DMACOSX -DPIPES -DNO_FLTK_THREADS -DHAVE_SOCKETS)
    find_library(ACCELERATE_LIBRARY Accelerate)
    find_path(VECLIB_PATH "Accelerate/Accelerate.h")
    include_directories(${VECLIB_PATH})
    list(APPEND libcsound_LIBS ${MATH_LIBRARY} dl ${ACCELERATE_LIBRARY})
endif()

if(WIN32)
    add_definitions(-DWIN32)
endif()

add_subdirectory(./src/chua)
add_subdirectory(./src/faustcsound)
add_subdirectory(./src/image)
add_subdirectory(./src/py)
add_subdirectory(./src/widgets)
add_subdirectory(./src/AbletonLinkOpcodes)
add_subdirectory(./src/stk)
add_subdirectory(./src/CUDA)
add_subdirectory(./src/opencl)
add_subdirectory(./src/websockets)
add_subdirectory(./src/hdf5)
add_subdirectory(./src/linear_algebra)
add_subdirectory(./src/mp3)
add_subdirectory(./src/wiimote)
add_subdirectory(./src/p5glove)
add_subdirectory(./src/jackops)
add_subdirectory(./src/fluidOpcodes)
