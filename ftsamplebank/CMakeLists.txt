# Build ftsamplebank opcodes
if(${BUILD_FTSAMPLEBANK_OPCODES})
    check_deps(BUILD_FTSAMPLEBANK_OPCODES)
    make_plugin(ftsamplebank ftsamplebank.cpp)
    target_include_directories(ftsamplebank PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
