option(BUILD_MP3OUT_OPCODE "Build the mp3out opcode" ON)

if(USE_VCPKG)
    find_package(mp3lame CONFIG REQUIRED)

    if(NOT TARGET mp3lame::mp3lame)
        message(STATUS "Could not find mp3lame")
        set(BUILD_MP3OUT_OPCODE OFF)
    endif()
else()
    find_package(LAME)
    check_deps(BUILD_MP3OUT_OPCODE mp3lame_FOUND)
endif()

if(BUILD_MP3OUT_OPCODE)
    if(USE_VCPKG)
        make_plugin(mp3out mp3out.c mp3lame::mp3lame)
    else()
        make_plugin(mp3out mp3out.c ${LAME_LIBRARY})
        target_include_directories(mp3out PRIVATE ${LAME_INCLUDE_DIR})
    endif()

    message(STATUS "Building MP3 opcodes")
endif()
