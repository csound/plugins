# Build Ableton Link Opcodes
option(BUILD_ABLETON_LINK_OPCODES "Build the Ableton link opcodes" OFF)

if(BUILD_ABLETON_LINK_OPCODES)
    message(STATUS "Ableton Link home: ${ABLETON_LINK_HOME}")

    if(USE_VCPKG)
        find_package(AbletonLink CONFIG REQUIRED PATH_SUFFIXES share/ableton-link)
    endif()

    if(APPLE)
        make_plugin(ableton_link_opcodes ableton_link_opcodes.cpp)
        target_link_libraries(ableton_link_opcodes PRIVATE Ableton::Link)
        set_target_properties(ableton_link_opcodes
            PROPERTIES
                COMPILE_FLAGS "-std=c++11 -stdlib=libc++ -Wno-multichar -DLINK_PLATFORM_MACOSX=1"
                LINK_FLAGS "-std=c++11 -stdlib=libc++"
        )
    elseif(LINUX)
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_CXX11)

        if(COMPILER_SUPPORTS_CXX11)
            make_plugin(ableton_link_opcodes ableton_link_opcodes.cpp)
            target_link_libraries(ableton_link_opcodes PRIVATE Ableton::Link)
            set_target_properties(ableton_link_opcodes PROPERTIES COMPILE_FLAGS "-std=gnu++11 -Wno-multichar -DLINK_PLATFORM_LINUX=1")
        else()
            message(STATUS "Not building ableton_link_opcodes as no C++11 support found.")
        endif()
    else()
        make_plugin(ableton_link_opcodes ableton_link_opcodes.cpp)

        if(WIN32)
            if(USE_VCPKG)
                target_link_libraries(ableton_link_opcodes PRIVATE Ableton::Link)
            else()
                set_target_properties(ableton_link_opcodes
                    PROPERTIES
                        COMPILE_FLAGS "-std=gnu++11 -Wno-multichar -DLINK_PLATFORM_WINDOWS=1"
                        LINK_FLAGS "-std=gnu++11"
                )
                target_link_libraries(ableton_link_opcodes wsock32 ws2_32 iphlpapi)
            endif()
        endif()
    endif()

    target_include_directories(ableton_link_opcodes
        PRIVATE
            ${CSOUND_INCLUDE_DIRS}
            ${ABLETON_LINK_HOME}/include
            ${ABLETON_LINK_HOME}/modules/asio-standalone/asio/include
    )

    install(TARGETS ableton_link_opcodes
        LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
        ARCHIVE DESTINATION ${LIBRARY_INSTALL_DIR}
    )
endif()
