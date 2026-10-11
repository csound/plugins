option(BUILD_HDF5_OPCODES "Build the hdf5 read/write opcodes" ON)

if(USE_VCPKG)
    find_package(HDF5 CONFIG)
else()
    find_package(HDF5)
endif()
check_deps(BUILD_HDF5_OPCODES HDF5_FOUND)

if(BUILD_HDF5_OPCODES AND HDF5_FOUND)
    make_plugin(hdf5ops HDF5IO.c)

    target_include_directories(hdf5ops PRIVATE ${HDF5_INCLUDE_DIRS} ${CSOUND_INCLUDE_DIRS})

    if(USE_VCPKG)
        target_link_libraries(hdf5ops PRIVATE hdf5::hdf5-static)
    else()
        message(STATUS "HDF5_INCLUDE_DIRS: ${HDF5_INCLUDE_DIRS}")
        message(STATUS "HDF5_LIBRARIES: ${HDF5_LIBRARIES}")

        target_link_libraries(hdf5ops ${HDF5_LIBRARIES})
    endif()
endif()
