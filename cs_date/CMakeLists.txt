# Build cs_date opcodes
if(${BUILD_CS_DATE_OPCODES})
    check_deps(BUILD_CS_DATE_OPCODES)
    make_plugin(cs_date date.c)
    target_include_directories(cs_date PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
