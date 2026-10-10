# Build fluidsynth opcodes
option(BUILD_FLUID_OPCODES "Build the fluidsynth opcodes" ON)

find_package(FLUIDSYNTH)
check_deps(BUILD_FLUID_OPCODES FLUIDSYNTH_FOUND)

if(BUILD_FLUID_OPCODES AND FLUIDSYNTH_FOUND)
    make_plugin(fluidOpcodes fluidOpcodes.cpp)
    target_include_directories(fluidOpcodes PRIVATE ${FLUIDSYNTH_INCLUDE_DIR} ${CSOUND_INCLUDE_DIRS})
    target_compile_definitions(fluidOpcodes PRIVATE BUILD_PLUGINS)
    target_link_libraries(fluidOpcodes PRIVATE ${FLUIDSYNTH_LIBRARIES})
endif()
