option(BUILD_HDF5_OPCODES "Build the hdf5 read/write opcodes" ON)

if(USE_VCPKG)
    find_package(HDF5 CONFIG REQUIRED)

    if(NOT TARGET hdf5::hdf5-static AND NOT TARGET hdf5::hdf5-shared)
        message(STATUS "Could not find hdf5")
        set(BUILD_HDF5_OPCODES OFF)
    endif()
else()
    find_package(HDF5)
    check_deps(BUILD_HDF5_OPCODES HDF5_FOUND)
endif()

if(BUILD_HDF5_OPCODES)
    # split version string, as cmake doesn't do this automatically
    string(REPLACE "." ";" VERSION_LIST ${HDF5_VERSION})

    list(GET VERSION_LIST 0 HDF5_VERSION_MAJOR)
    list(GET VERSION_LIST 1 HDF5_VERSION_MINOR)
    list(GET VERSION_LIST 2 HDF5_VERSION_PATCH)

    make_plugin(hdf5ops HDF5IO.c)

    target_compile_definitions(hdf5ops PRIVATE
        -DHDF5_VERSION_MAJOR=${HDF5_VERSION_MAJOR}
        -DHDF5_VERSION_MINOR=${HDF5_VERSION_MINOR}
        -DHDF5_VERSION_PATCH=${HDF5_VERSION_PATCH}
    )

    if(USE_VCPKG)
        target_link_libraries(hdf5ops PRIVATE
            $<IF:$<TARGET_EXISTS:hdf5::hdf5-static>,hdf5::hdf5-static,hdf5::hdf5-shared>
            $<IF:$<TARGET_EXISTS:hdf5::hdf5_hl-static>,hdf5::hdf5_hl-static,hdf5::hdf5_hl-shared>
        )
    else()
        message(STATUS "HDF5_INCLUDE_DIRS: ${HDF5_INCLUDE_DIRS}")
        message(STATUS "HDF5_LIBRARIES: ${HDF5_LIBRARIES}")

        target_include_directories(hdf5ops PRIVATE ${HDF5_INCLUDE_DIRS})
        target_link_libraries(hdf5ops ${HDF5_LIBRARIES})
    endif()

    message(STATUS "Building HDF5 opcodes")
endif()
