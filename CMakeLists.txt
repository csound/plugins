cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

option(USE_VCPKG "Use VCPKG to download and manage dependencies" OFF)

if(USE_VCPKG)
    # Detect correct triplet based on CMake env
    if (WIN32)
        set(VCPKG_HOST_TRIPLET "x64-windows-static-md" CACHE STRING "")
        set(VCPKG_TARGET_TRIPLET "x64-windows-static-md")
    elseif(APPLE)
        set(VCPKG_HOST_TRIPLET "x64-osx" CACHE STRING "")
        set(VCPKG_TARGET_TRIPLET "x64-osx")
    elseif (UNIX)
        set(VCPKG_HOST_TRIPLET "x64-linux" CACHE STRING "")
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()

    set(VCPKG_FEATURE_FLAGS versions)
    set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}")
    set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_SOURCE_DIR}")
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(Csound-plugins)

set(APIVERSION "6.0")

add_subdirectory(./6.x)
add_subdirectory(./7.x)
