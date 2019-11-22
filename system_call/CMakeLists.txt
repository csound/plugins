# Build system_call opcodes
if(${BUILD_SYSTEM_CALL_OPCODES})
    check_deps(BUILD_SYSTEM_CALL_OPCODES)
    make_plugin(system_call system_call.c)
    target_include_directories(system_call PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
