# Build getftargs opcodes
if(${BUILD_LIVECONV_OPCODES})
    check_deps(BUILD_LIVECONV_OPCODES)
    make_plugin(liveconv liveconv.c)
    target_include_directories(liveconv PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
