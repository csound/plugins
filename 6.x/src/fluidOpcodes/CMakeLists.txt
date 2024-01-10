option(BUILD_FLUID_OPCODES "Build the fluidsynth opcodes" ON)

if(USE_VCPKG)
    find_package(FluidSynth CONFIG REQUIRED)
else()
    find_package(FLUIDSYNTH)
    check_deps(BUILD_FLUID_OPCODES FLUIDSYNTH_FOUND)
endif()

if(NOT TARGET FluidSynth::libfluidsynth)
    message(STATUS "Could not find FluidSynth::libfluidsynth")
    set(BUILD_FLUID_OPCODES OFF)
endif()

if(BUILD_FLUID_OPCODES)
    make_plugin(fluidOpcodes fluidOpcodes.cpp)

    if(USE_VCPKG)
        target_link_libraries(fluidOpcodes PRIVATE FluidSynth::libfluidsynth)
    else()
        target_include_directories(fluidOpcodes PRIVATE ${FLUIDSYNTH_INCLUDE_DIR})
        target_link_libraries(fluidOpcodes ${FLUIDSYNTH_LIBRARIES})
    endif()
endif()
