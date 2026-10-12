# Build the OpenCL opcodes.
#
# The OpenCL headers and ICD loader come from the system framework on macOS,
# from vcpkg on Windows, and from ocl-icd-opencl-dev on Linux.
option(BUILD_OPENCL_OPCODES "Build the OpenCL opcodes" ON)

if(APPLE OR NOT USE_VCPKG)
    find_package(OpenCL)
else()
    find_package(OpenCL CONFIG)
endif()

check_deps(BUILD_OPENCL_OPCODES OpenCL_FOUND)

if(BUILD_OPENCL_OPCODES AND OpenCL_FOUND)
    make_plugin(clop cladsynth.c)
    target_include_directories(clop PRIVATE ${CSOUND_INCLUDE_DIRS})
    if(TARGET OpenCL::OpenCL)
        target_link_libraries(clop PRIVATE OpenCL::OpenCL)
    else()
        target_include_directories(clop PRIVATE ${OpenCL_INCLUDE_DIRS})
        target_link_libraries(clop PRIVATE ${OpenCL_LIBRARIES})
    endif()
endif()
