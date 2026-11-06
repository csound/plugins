# Build chips opcodes (floooh/chips emulators)
option(BUILD_CHIPS_OPCODES "Build the chips opcodes" ON)

if(BUILD_CHIPS_OPCODES)
    # m6581.h is header-only and vendored under this directory.
    make_plugin(chips sid6581.cpp)
    target_include_directories(chips PRIVATE
        ${CSOUND_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(chips PRIVATE BUILD_PLUGINS)

    message(STATUS "Building chips opcodes")
endif()
