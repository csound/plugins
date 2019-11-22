# Build mixer opcodes
if(${BUILD_MIXER_OPCODES})
    check_deps(BUILD_MIXER_OPCODES)
    make_plugin(mixer mixer.cpp)
    target_include_directories(mixer PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
