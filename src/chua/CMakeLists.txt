# Build chuap opcodes
option(BUILD_CHUA_OPCODES "Build the chua opcodes" ON)

if(USE_VCPKG)
    find_package(Eigen3 CONFIG REQUIRED)

    if(NOT TARGET Eigen3::Eigen)
        message(STATUS "Could not find Eigen3::Eigen")
        set(BUILD_CHUA_OPCODES OFF)
    endif()
else()
    find_package(EIGEN3)
    check_deps(BUILD_CHUA_OPCODES EIGEN3_FOUND)
endif()

if(BUILD_CHUA_OPCODES)
    make_plugin(chua ChuaOscillator.cpp)

    if(USE_VCPKG)
        target_link_libraries(chua PRIVATE Eigen3::Eigen)
    else()
        target_include_directories(chua PRIVATE ${EIGEN3_INCLUDE_DIR})
        target_link_libraries(chua PRIVATE ${EIGEN_LIBRARIES})
    endif()

    if(APPLE)
        set_target_properties(chua PROPERTIES
            COMPILE_FLAGS "-Wno-unused-local-typedef"
        )
    endif()

    message(STATUS "Building Chua opcodes")
endif()
