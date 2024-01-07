# Build faustcsound opcodes
option(BUILD_FAUST_OPCODES "Build the Faust opcodes" ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
find_package(FAUST)

check_deps(BUILD_FAUST_OPCODES FAUST_FOUND Threads_FOUND)
if(BUILD_FAUST_OPCODES)
    make_plugin(faustcsound faustgen.cpp)

    if(APPLE)
        set_target_properties(faustcsound PROPERTIES
            INSTALL_RPATH_USE_LINK_PATH TRUE
        )
    endif()

    target_include_directories(faustcsound PRIVATE ${FAUST_INCLUDE_DIR})
    target_compile_definitions(faustcsound PRIVATE "-DFAUSTFLOAT=MYFLT")
    target_compile_features(faustcsound PRIVATE cxx_std_11)
    target_link_libraries(faustcsound Threads::Threads ${FAUST_LIBRARIES})
endif()
