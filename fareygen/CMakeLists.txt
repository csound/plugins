# Build fareygen opcodes
if(${BUILD_FAREYGEN_OPCODES})
    check_deps(BUILD_FAREYGEN_OPCODES)
    make_plugin(fareygen fareygen.c)
    target_include_directories(fareygen PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
