# Build gtf opcodes
if(${BUILD_GTF_OPCODES})
    check_deps(BUILD_GTF_OPCODES)
    make_plugin(gtf gammatone.c)
    target_include_directories(gtf PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
