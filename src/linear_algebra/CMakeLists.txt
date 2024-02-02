option(BUILD_LINEAR_ALGEBRA_OPCODES "Build the linear algebra opcodes" ON)

find_package(GMM)
check_deps(BUILD_LINEAR_ALGEBRA_OPCODES GMM_FOUND)

if(BUILD_LINEAR_ALGEBRA_OPCODES AND GMM_FOUND)
    make_plugin(linear_algebra linear_algebra.cpp)
    target_include_directories(linear_algebra PRIVATE ${GMM_INCLUDE_DIR})
    target_compile_options(linear_algebra PRIVATE "-DGMM_VERSION")

    if (MSVC)
        target_compile_options(linear_algebra PRIVATE "/D_SCL_SECURE_NO_DEPRECATE")
    endif()

    message(STATUS "Building linear algebra opcodes")
endif()
