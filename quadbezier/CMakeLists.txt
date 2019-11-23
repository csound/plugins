# Build quadbezier opcodes
if(${BUILD_QUADBEZIER_OPCODES})
    check_deps(BUILD_QUADBEZIER_OPCODES)
    make_plugin(quadbezier quadbezier.c)
    target_include_directories(quadbezier PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
