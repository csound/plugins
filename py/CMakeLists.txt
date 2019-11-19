# Build Python Opcodes
find_package(PythonLibs 3)

string(REGEX MATCH "python3\\.." PYTHON3_SUBVERSION ${PYTHON_LIBRARIES})
set(PYTHON_INCLUDE_DIRS /usr/include/${PYTHON3_SUBVERSION})

add_library(py MODULE src/pythonopcodes.c)   
target_include_directories(py PRIVATE ${CSOUND_INCLUDE_DIRS})
target_include_directories(py PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(py ${PYTHON_LIBRARIES})
set_target_properties(py PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}  
  LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})
                                   
