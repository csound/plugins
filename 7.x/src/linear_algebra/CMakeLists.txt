option(BUILD_LINEAR_ALGEBRA_OPCODES "Build the linear algebra opcodes" ON)

# gmm++ is header-only and vendored under this directory as gmm/gmm.h.
# GMM_INCLUDE_PATH can be overridden to use a system gmm++ installation.
set(GMM_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to the gmm++ headers")

find_package(GMM)
check_deps(BUILD_LINEAR_ALGEBRA_OPCODES GMM_FOUND)

if(BUILD_LINEAR_ALGEBRA_OPCODES AND GMM_FOUND)
    make_plugin(linear_algebra linear_algebra.cpp)
    target_include_directories(linear_algebra PRIVATE ${GMM_INCLUDE_DIR} ${CSOUND_INCLUDE_DIRS})
    target_compile_options(linear_algebra PRIVATE "-DGMM_VERSION")
    target_compile_definitions(linear_algebra PRIVATE BUILD_PLUGINS)

    if (MSVC)
        target_compile_options(linear_algebra PRIVATE "/D_SCL_SECURE_NO_DEPRECATE")
    endif()

    message(STATUS "Building linear algebra opcodes")
endif()
