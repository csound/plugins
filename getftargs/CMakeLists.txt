# Build getftargs opcodes
if(${BUILD_GETFTARGS_OPCODES})
    check_deps(BUILD_GETFTARGS_OPCODES)
    make_plugin(getftargs getftargs.c)
    target_include_directories(getftargs PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
