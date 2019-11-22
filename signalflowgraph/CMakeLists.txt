# Build signalflowgraph opcodes
if(${BUILD_SIGNALFLOWGRAPH_OPCODES})
    check_deps(BUILD_SIGNALFLOWGRAPH_OPCODES)
    make_plugin(signalflowgraph signalflowgraph.cpp)
    target_include_directories(signalflowgraph PRIVATE ${CSOUND_INCLUDE_DIRS})
endif()
