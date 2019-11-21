# Build Python opcodes
if(${BUILD_PYTHON_OPCODES})
    find_package(PythonLibs 3)
    
    string(REGEX MATCH "python3\\.." PYTHON3_SUBVERSION ${PYTHON_LIBRARIES})
    set(PYTHON_INCLUDE_DIRS /usr/include/${PYTHON3_SUBVERSION})
    
    check_deps(BUILD_PYTHON_OPCODES PYTHONLIBS_FOUND)
    if(BUILD_PYTHON_OPCODES)
	make_plugin(py src/pythonopcodes.c)
	target_include_directories(py PRIVATE ${CSOUND_INCLUDE_DIRS})
	target_include_directories(py PRIVATE ${PYTHON_INCLUDE_DIRS})
	target_link_libraries(py ${PYTHON_LIBRARIES})
    endif()
endif()                                                                                 