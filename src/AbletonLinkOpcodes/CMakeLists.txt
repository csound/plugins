# Build Ableton Link Opcodes
option(BUILD_ABLETON_LINK_OPCODES "Build the Ableton link opcodes" ON)

if(USE_VCPKG)
    find_package(AbletonLink CONFIG REQUIRED)

    if(NOT TARGET Ableton::Link)
        message(STATUS "Could not find Ableton::Link")
        set(BUILD_ABLETON_LINK_OPCODES OFF)
    endif()
else()
    if(NOT "${ABLETON_LINK_HOME}" STREQUAL "")
        include("${ABLETON_LINK_HOME}/AbletonLinkConfig.cmake")
    endif()
endif()

if(BUILD_ABLETON_LINK_OPCODES)
    make_plugin(ableton_link_opcodes ableton_link_opcodes.cpp Ableton::Link)

    target_include_directories(ableton_link_opcodes
        PRIVATE
            "${ABLETON_LINK_HOME}/include"
            "${ABLETON_LINK_HOME}/modules/asio-standalone/asio/include"
    )

    target_compile_features(ableton_link_opcodes PUBLIC cxx_std_11)

    if(UNIX)
        target_compile_options(ableton_link_opcodes PRIVATE "-Wno-multichar")
    endif()

    if(WIN32)
        target_link_libraries(ableton_link_opcodes PRIVATE wsock32 ws2_32 iphlpapi)
    endif()

    install(TARGETS ableton_link_opcodes
        LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
    )

    message(STATUS "Building Ableton opcodes")
endif()
