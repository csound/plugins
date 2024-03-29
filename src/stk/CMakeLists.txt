option(BUILD_STK_OPCODES "Build the stk opcodes" ON)

if(BUILD_STK_OPCODES)
    if(DEFAULT_STK_RAWWAVE_PATH)
        set(rawwave_path "-DDEFAULT_RAWWAVE_PATH=\"${DEFAULT_STK_RAWWAVE_PATH}\"")
    endif()

    if(USE_VCPKG)
        make_plugin(stkops stkOpcodes.cpp)
        find_package(unofficial-libstk CONFIG REQUIRED)
        target_link_libraries(stkops PRIVATE unofficial::libstk::libstk)
        target_include_directories(stkops PRIVATE ${CSOUND_INCLUDE_DIRS})
        message(STATUS "Building STK opcodes for MSVC")
    else()
        find_package(STK)

        if(STK_LIBRARY)
            find_path(STK_INCLUDE_DIR Stk.h
                /usr/include/stk
                /usr/local/include/stk)

            check_deps(BUILD_STK_OPCODES STK_INCLUDE_DIR)

            if(STK_INCLUDE_DIR)
                make_plugin(stkops stkOpcodes.cpp)
                target_include_directories(stkops PRIVATE ${CSOUND_INCLUDE_DIRS} ${STK_INCLUDE_DIR})
                message(STATUS "Building STK opcodes.")

                if(BUILD_STATIC_LIBRARY)
                    if(WIN32)
                        add_library(stk STATIC IMPORTED)
                        set_target_properties(stk PROPERTIES IMPORTED_LOCATION ${STK_LIBRARY})
                        target_link_libraries(stkops stk)
                    else()
                        target_link_libraries(stkops ${STK_LIBRARY})
                    endif()
                else()
                target_link_libraries(stkops ${STK_LIBRARIES} ${PTHREAD_LIBRARY})
                endif()

                target_compile_definitions(stkops PRIVATE ${rawwave_path})
            endif()
        else()
            set(stk_remove_srcs
            src/InetWvIn.cpp    src/InetWvOut.cpp
            src/Mutex.cpp       src/RtAudio.cpp
            src/RtMidi.cpp      src/Messager.cpp
            src/RtWvIn.cpp      src/RtWvOut.cpp
            src/Socket.cpp      src/TcpClient.cpp
            src/TcpServer.cpp   src/Thread.cpp
            src/UdpSocket.cpp
        )

        file(GLOB stk_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*)
        list(REMOVE_ITEM stk_srcs ${stk_remove_srcs})
        list(APPEND stk_srcs stkOpcodes.cpp)

        # Assume that if this file exists, then we have the sources
        find_file(STK_FOUND
            NAMES src/InetWvIn.cpp
            HINTS ${CMAKE_CURRENT_SOURCE_DIR}
        )

        find_path(STK_INCLUDE_DIR Stk.h ./include)
        check_deps(BUILD_STK_OPCODES STK_FOUND STK_INCLUDE_DIR)

        make_plugin(stkops "${stk_srcs}")
        target_include_directories(stkops PRIVATE
            ${CSOUND_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include/stk
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
        set(stkdefs "-D__STK_REALTIME__")

        if(APPLE)
            list(APPEND stkdefs "-D__OS_MACOSX__")
            list(REMOVE_ITEM stkdefs "-D__STK_REALTIME__")
        elseif(LINUX)
            list(APPEND stkdefs "-D__OS_LINUX__")
            list(APPEND stkdefs "-D__LINUX_ALSA__")
        elseif(WIN32)
            list(APPEND stkdefs "-D__OS_WINDOWS__")
        endif()

        if(BIG_ENDIAN)
            list(APPEND stkdefs "-D__BIG_ENDIAN__")
        else()
            list(APPEND stkdefs "-D__LITTLE_ENDIAN__")
        endif()

        target_compile_definitions(stkops PRIVATE ${stkdefs} ${rawwave_path})
        endif()
    endif()
endif()

