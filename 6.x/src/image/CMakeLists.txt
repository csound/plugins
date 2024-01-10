# Build image opcodes
option(BUILD_IMAGE_OPCODES "Build the image opcodes" ON)

if(USE_VCPKG)
    find_package(PNG REQUIRED)
else()
    find_package(PNG)
    check_deps(BUILD_IMAGE_OPCODES PNG_FOUND)
endif()

if(NOT TARGET PNG::PNG)
    message(STATUS "Could not find libpng")
    set(BUILD_IMAGE_OPCODES OFF)
endif()

if(BUILD_IMAGE_OPCODES)
    make_plugin(image imageOpcodes.c)

    if(USE_VCPKG)
        target_link_libraries(image PRIVATE PNG::PNG)
    else()
        target_compile_definitions(image PRIVATE ${PNG_DEFINITIONS})
        target_include_directories(image PRIVATE ${PNG_INCLUDE_DIR})
        target_link_libraries(image ${PNG_LIBRARIES})
    endif()
endif()
