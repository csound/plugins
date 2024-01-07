option(BUILD_FLUID_OPCODES "Build the fluidsynth opcodes" ON)

find_package(FLUIDSYNTH)
check_deps(BUILD_FLUID_OPCODES FLUIDSYNTH_FOUND)

if(BUILD_FLUID_OPCODES)
    make_plugin(fluidOpcodes fluidOpcodes.cpp)
    target_include_directories(fluidOpcodes PRIVATE ${FLUIDSYNTH_INCLUDE_DIR})
    target_link_libraries(fluidOpcodes ${FLUIDSYNTH_LIBRARIES})
endif()
