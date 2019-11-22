# Build cellular opcodes
if(${BUILD_CELLULAR_OPCODES})
    check_deps(BUILD_CELLULAR_OPCODES)
    make_plugin(cellular cellular.c)
    target_include_directories(cellular PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
