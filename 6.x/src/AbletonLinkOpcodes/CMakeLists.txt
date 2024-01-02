# Build Ableton Link Opcodes
option(BUILD_ABLETON_LINK_OPCODES "Build the Ableton link opcodes" OFF)

if(USE_VCPKG)
    find_package(AbletonLink CONFIG REQUIRED PATH_SUFFIXES share/ableton-link)
else()
    if(NOT "${ABLETON_LINK_HOME}" STREQUAL "")
        include("${ABLETON_LINK_HOME}/AbletonLinkConfig.cmake")
    endif()
endif()

if(NOT Ableton::Link)
    message(STATUS "Could not find Ableton::Link")
    set(BUILD_ABLETON_LINK_OPCODES OFF)
endif()

if(BUILD_ABLETON_LINK_OPCODES)
    message(STATUS "Ableton Link home: ${ABLETON_LINK_HOME}")

    make_plugin(ableton_link_opcodes ableton_link_opcodes.cpp Ableton::Link)
    target_compile_features(ableton_link_opcodes PUBLIC cxx_std_11)
    target_compile_options(ableton_link_opcodes PRIVATE "-Wno-multichar")

    if(APPLE)
        set_target_properties(ableton_link_opcodes
            PROPERTIES
                COMPILE_FLAGS "-stdlib=libc++ -DLINK_PLATFORM_MACOSX=1"
                LINK_FLAGS "-stdlib=libc++"
        )
    elseif(LINUX)
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)

        if(COMPILER_SUPPORTS_CXX11)
            set_target_properties(ableton_link_opcodes
                PROPERTIES
                    COMPILE_FLAGS "-DLINK_PLATFORM_LINUX=1"
            )
        else()
            message(STATUS "Not building ableton_link_opcodes as no C++11 support found.")
        endif()
    else()
        if(WIN32)
            set_target_properties(ableton_link_opcodes
                PROPERTIES
                    COMPILE_FLAGS "-DLINK_PLATFORM_WINDOWS=1"
            )

            target_link_libraries(ableton_link_opcodes wsock32 ws2_32 iphlpapi)
        endif()
    endif()

    target_include_directories(ableton_link_opcodes
        PRIVATE
            "${ABLETON_LINK_HOME}/include"
            "${ABLETON_LINK_HOME}/modules/asio-standalone/asio/include"
    )

    install(TARGETS ableton_link_opcodes
        LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
        ARCHIVE DESTINATION "${LIBRARY_INSTALL_DIR}"
    )
endif()
