# Build fluidsynth opcodes
option(BUILD_FLUID_OPCODES "Build the fluidsynth opcodes" ON)

# Prefer the CMake config package (e.g. from vcpkg or a system install), which
# carries the required transitive dependencies (system frameworks, glib, ...).
find_package(FluidSynth CONFIG QUIET)
if(TARGET FluidSynth::libfluidsynth)
    set(FLUIDSYNTH_FOUND TRUE)
    set(FLUIDSYNTH_LINK_LIBRARIES FluidSynth::libfluidsynth)
else()
    find_package(FLUIDSYNTH)
    set(FLUIDSYNTH_LINK_LIBRARIES ${FLUIDSYNTH_LIBRARIES})
endif()
check_deps(BUILD_FLUID_OPCODES FLUIDSYNTH_FOUND)

if(BUILD_FLUID_OPCODES AND FLUIDSYNTH_FOUND)
    make_plugin(fluidOpcodes fluidOpcodes.cpp)
    target_include_directories(fluidOpcodes PRIVATE ${FLUIDSYNTH_INCLUDE_DIR} ${CSOUND_INCLUDE_DIRS})
    target_compile_definitions(fluidOpcodes PRIVATE BUILD_PLUGINS)
    target_link_libraries(fluidOpcodes PRIVATE ${FLUIDSYNTH_LINK_LIBRARIES})
endif()
