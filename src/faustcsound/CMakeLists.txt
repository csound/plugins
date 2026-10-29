# Build faustcsound opcodes
option(BUILD_FAUST_OPCODES "Build the Faust opcodes" ON)

find_package(FAUST)
check_deps(BUILD_FAUST_OPCODES FAUST_FOUND)

if(BUILD_FAUST_OPCODES AND FAUST_FOUND)
    message(STATUS "Faust library: ${FAUST_LIBRARY}")

    make_plugin(faustcsound faustgen.cpp)

    target_include_directories(faustcsound PRIVATE
        ${CSOUND_INCLUDE_DIRS}
        ${FAUST_INCLUDE_DIR})
    target_compile_definitions(faustcsound PRIVATE "-DFAUSTFLOAT=cs_float")

    if(PTHREAD_LIBRARY)
        target_compile_definitions(faustcsound PRIVATE "-DHAVE_PTHREAD")
    endif()

    target_link_libraries(faustcsound
        ${FAUST_LIBRARIES}
        ${PTHREAD_LIBRARY}
        ${CMAKE_DL_LIBS})

    if(APPLE)
        set_target_properties(faustcsound PROPERTIES
            INSTALL_RPATH_USE_LINK_PATH TRUE
        )
    endif()

    if(FAUST_STATIC)
        # libfaustwithllvm bundles LLVM; without this the plugin would export
        # the hundreds of thousands of LLVM/Faust symbols from the archive.
        if(APPLE)
            set_target_properties(faustcsound PROPERTIES LINK_FLAGS
                "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/faustcsound.exports")
        elseif(UNIX)
            set_target_properties(faustcsound PROPERTIES LINK_FLAGS
                "-Wl,--exclude-libs,ALL")
        endif()
    endif()

    # Install the Faust standard library next to the plugin and point the
    # plugin at it, so programs using import("stdfaust.lib") work without a
    # separate Faust installation.
    if(FAUST_STDLIB_DIR)
        message(STATUS "Faust standard library: ${FAUST_STDLIB_DIR}")
        install(DIRECTORY "${FAUST_STDLIB_DIR}/"
                DESTINATION "${PLUGIN_INSTALL_DIR}/faust")
        target_compile_definitions(faustcsound PRIVATE
            "DEFAULT_FAUST_LIB_PATH=\"${PLUGIN_INSTALL_DIR}/faust\"")
    else()
        message(WARNING "Faust standard library (stdfaust.lib) not found; "
                        "set FAUST_STDLIB_DIR_HINT to enable import(\"stdfaust.lib\")")
    endif()
endif()
