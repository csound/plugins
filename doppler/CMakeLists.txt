# Build doppler opcodes
if(${BUILD_DOPPLER_OPCODES})
    check_deps(BUILD_DOPPLER_OPCODES)
    make_plugin(doppler doppler.cpp)
    target_include_directories(doppler PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
