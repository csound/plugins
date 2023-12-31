# Build faustcsound opcodes
option(BUILD_FAUST_OPCODES "Build the Faust opcodes" ON)

find_package(FAUST)
check_deps(BUILD_FAUST_OPCODES FAUST_FOUND)

if(BUILD_FAUST_OPCODES AND FAUST_FOUND)
    make_plugin(faustcsound faustgen.cpp)

    if(APPLE)
        set_target_properties(faustcsound PROPERTIES
            INSTALL_RPATH_USE_LINK_PATH TRUE
        )
    endif()

    target_link_libraries(faustcsound ${PTHREAD_LIBRARY})
    target_compile_definitions(faustcsound PRIVATE "-DFAUSTFLOAT=MYFLT")

    if(PTHREAD_LIBRARY)
        target_compile_definitions(faustcsound PRIVATE "-DHAVE_PTHREAD")
    endif()

    if(APPLE)
        if(${OSX_VERSION} STREQUAL "10.6")
            target_link_libraries(faustcsound ${FAUST_LIBRARIES})
        else()
            target_compile_options(faustcsound PRIVATE "-stdlib=libc++")
            target_link_libraries(faustcsound ${FAUST_LIBRARIES} -stdlib=libc++)
        endif()
    else()
        target_link_libraries(faustcsound ${FAUST_LIBRARIES})
    endif()

    target_include_directories(faustcsound PRIVATE ${FAUST_INCLUDE_DIR})
    message("-- FAUST INCLUDE DIR: ${FAUST_INCLUDE_DIR}")
endif()
