# Build chuap opcodes
option(BUILD_CHUA_OPCODES "Build the chua opcodes" ON)

if(USE_VCPKG)
    find_package(Eigen3 CONFIG REQUIRED)
else()
    find_package(EIGEN3)
endif()

check_deps(BUILD_CHUA_OPCODES EIGEN3_FOUND)

if(BUILD_CHUA_OPCODES AND EIGEN3_FOUND)
    make_plugin(chua ChuaOscillator.cpp)
    target_include_directories(chua PRIVATE ${EIGEN3_INCLUDE_DIR})

    if(USE_VCPKG)
        target_link_libraries(chua PRIVATE Eigen3::Eigen)
    else()
        target_link_libraries(chua PRIVATE ${EIGEN_LIBRARIES})
    endif()

    if(APPLE)
        set_target_properties(chua PROPERTIES
            COMPILE_FLAGS "-Wno-unused-local-typedef"
        )
    endif()
endif()
