# Build image opcodes
option(BUILD_IMAGE_OPCODES "Build the image opcodes" ON)

find_package(PNG)
check_deps(BUILD_IMAGE_OPCODES PNG_FOUND)

if(BUILD_IMAGE_OPCODES AND PNG_FOUND)
    make_plugin(image imageOpcodes.c)
    target_include_directories(image PRIVATE ${CSOUND_INCLUDE_DIRS})
    target_compile_definitions(image PRIVATE BUILD_PLUGINS)
    target_link_libraries(image PRIVATE PNG::PNG)
endif()
