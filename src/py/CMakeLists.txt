# Build py opcodes
option(BUILD_PYTHON_OPCODES "Build the py opcodes" ON)

if(BUILD_PYTHON_OPCODES)
    find_package(Python3 COMPONENTS Interpreter Development)
    check_deps(BUILD_PYTHON_OPCODES Python3_FOUND)

    set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
    set(PYTHON_LIBRARIES ${Python3_LIBRARIES})

    message("-- Python include dirs: ${Python3_INCLUDE_DIRS}")
    message("-- Python libraries: ${PYTHON_LIBRARIES}")

    make_plugin(py src/pythonopcodes.c)
    target_include_directories(py PRIVATE ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(py ${PYTHON_LIBRARIES})
endif()
