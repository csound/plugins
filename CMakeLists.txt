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

option(BUILD_CELLULAR_OPCODES "Build the cellular opcodes" ON)
option(BUILD_CHUA_OPCODES "Build the chua opcodes" ON)
option(BUILD_DOPPLER_OPCODES "Build the doppler opcodes" ON)
option(BUILD_IMAGE_OPCODES "Build the image opcodes" ON)
option(BUILD_MIXER_OPCODES "Build the mixer opcodes" ON)
option(BUILD_PYTHON_OPCODES "Build the Python opcodes" ON)

add_subdirectory(cellular)
add_subdirectory(chua)
add_subdirectory(doppler)
add_subdirectory(image)
add_subdirectory(mixer)
add_subdirectory(py)
