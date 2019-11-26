cmake_minimum_required(VERSION 2.8.12)
project(Csound-plugins)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_COMPILER_IS_CLANG 1)
endif()

# C++11 needed
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

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

find_package(Csound)

option(USE_LIB64 "Set to on to set installation directory for libraries to lib64" OFF)
if(USE_LIB64)
    set(LIBRARY_INSTALL_DIR "lib64")
    add_definitions("-DLIB64")
else()
    set(LIBRARY_INSTALL_DIR "lib")
endif()
message(STATUS "LIBRARY INSTALL DIR: ${LIBRARY_INSTALL_DIR}")

if(USE_DOUBLE)
    message(STATUS "Building with 64-bit floats")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins64-${APIVERSION}")
    if(APPLE)
        set(PLUGIN_INSTALL_DIR "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes64")
    endif()
else()
    message(STATUS "Building with 32-bit floats")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins-${APIVERSION}")
    if(APPLE)
        set(PLUGIN_INSTALL_DIR "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes")
    endif()
endif()


# Checks if dependencies for an enabled target are fulfilled.
# If FAIL_MISSING is true and the dependencies are not fulfilled,
# it will abort the cmake run.
# If FAIL_MISSING is false, it will set the option to OFF.
# If the target is not enabled, it will do nothing.
# example: check_deps(BUILD_NEW_PARSER FLEX_EXECUTABLE BISON_EXECUTABLE)
function(check_deps option)
    if(${option})
        set(i 1)
        while( ${i} LESS ${ARGC} )
            set(dep ${ARGV${i}})
            if(NOT ${dep})
                if(FAIL_MISSING)
                    message(FATAL_ERROR
                        "${option} is enabled, but ${dep}=\"${${dep}}\"")
                else()
                    message(STATUS "${dep}=\"${${dep}}\", so disabling ${option}")
                    set(${option} OFF PARENT_SCOPE)
                    # Set it in the local scope too
                    set(${option} OFF)
                endif()
            endif()
            math(EXPR i "${i}+1")
        endwhile()
    endif()
    if(${option})
        message(STATUS "${option} is enabled.")
    else()
        message(STATUS "${option} is disabled.")
    endif()
endfunction(check_deps)

# Utility function to make plugins. All plugin targets should use this as it
# sets up output directory set in top-level CmakeLists.txt
# and adds the appropriate install target
#
# libname - name of library to produce
# srcs - list of src files (must be quoted if a list)
# extralibs (OPTIONAL) - extra libraries to link the plugin to
#
# NB - this was moved here as it needs some VARS defined above
# for setting up the framework
function(make_plugin libname srcs)
    if(APPLE)
        add_library(${libname} SHARED ${srcs})
    else()
        add_library(${libname} MODULE ${srcs})
    endif()

    set(i 2)
    while( ${i} LESS ${ARGC} )
        target_link_libraries(${libname} ${ARGV${i}})
        math(EXPR i "${i}+1")
    endwhile()

    set_target_properties(${libname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    install(TARGETS ${libname}
        LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}" 
        ARCHIVE DESTINATION "${PLUGIN_INSTALL_DIR}" )
endfunction(make_plugin)

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
    message(STATUS "Building on OSX")
    add_definitions(-DMACOSX -DPIPES -DNO_FLTK_THREADS -DHAVE_SOCKETS)
    find_library(ACCELERATE_LIBRARY Accelerate)
    find_path(VECLIB_PATH "Accelerate/Accelerate.h")
    include_directories(${VECLIB_PATH})
    list(APPEND libcsound_LIBS ${MATH_LIBRARY} dl ${ACCELERATE_LIBRARY})
endif()

if(WIN32)
    add_definitions(-DWIN32)
endif()


option(BUILD_CHUA_OPCODES "Build the chua opcodes" ON)
option(BUILD_FAUST_OPCODES "Build the Faust opcodes" ON)
option(BUILD_IMAGE_OPCODES "Build the image opcodes" ON)
option(BUILD_PYTHON_OPCODES "Build the py opcodes" ON)

option(USE_FLTK "Use FLTK for graphs and widget opcodes" ON)

add_subdirectory(chua)
add_subdirectory(faustcsound)
add_subdirectory(image)
add_subdirectory(py)
add_subdirectory(widgets)
