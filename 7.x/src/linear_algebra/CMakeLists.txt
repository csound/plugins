option(BUILD_LINEAR_ALGEBRA_OPCODES "Build the linear algebra opcodes" ON)

# gmm++ is header-only and vendored under this directory as gmm/gmm.h.
# GMM_INCLUDE_PATH can be overridden to use a system gmm++ installation.
set(GMM_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "Path to the gmm++ headers")

find_package(GMM)
check_deps(BUILD_LINEAR_ALGEBRA_OPCODES GMM_FOUND)

if(BUILD_LINEAR_ALGEBRA_OPCODES AND GMM_FOUND)
    make_plugin(linear_algebra linear_algebra.cpp)
    target_include_directories(linear_algebra PRIVATE ${GMM_INCLUDE_DIR} ${CSOUND_INCLUDE_DIRS})
    target_compile_definitions(linear_algebra PRIVATE BUILD_PLUGINS)

    # gmm++ 5.4.3 made gmm::lapack_ipvt a typedef of std::vector and dropped its
    # get() accessor (elements are now accessed with operator[]). There is no
    # preprocessor macro to detect this, so probe the headers that were found.
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES ${GMM_INCLUDE_DIR})
    check_cxx_source_compiles("
      #include <gmm/gmm.h>
      int main() {
        gmm::lapack_ipvt pivot(1);
        return (int)pivot.get(0);
      }
    " GMM_LAPACK_IPVT_GET)
    unset(CMAKE_REQUIRED_INCLUDES)

    if(GMM_LAPACK_IPVT_GET)
        target_compile_definitions(linear_algebra PRIVATE HAVE_LAPACK_IPVT_GET=1)
    endif()

    if (MSVC)
        target_compile_options(linear_algebra PRIVATE "/D_SCL_SECURE_NO_DEPRECATE")
    endif()

    message(STATUS "Building linear algebra opcodes")
endif()
