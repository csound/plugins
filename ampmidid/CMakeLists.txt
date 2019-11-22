# Build ampmidid opcodes
if(${BUILD_AMPMIDID_OPCODES})
    check_deps(BUILD_AMPMIDID_OPCODES)
    make_plugin(ampmidid ampmidid.cpp)
    target_include_directories(ampmidid PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
