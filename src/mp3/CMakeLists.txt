option(BUILD_MP3OUT_OPCODE "Build the mp3out opcode" ON)

if(USE_VCPKG)
    find_package(mp3lame CONFIG REQUIRED)
else()
    find_package(LAME)
endif()

if(BUILD_MP3OUT_OPCODE)
    if(LAME_FOUND)
        make_plugin(mp3out mp3out.c ${LAME_LIBRARY})
        target_include_directories(mp3out PRIVATE ${LAME_INCLUDE_DIR} ${CSOUND_INCLUDE_DIR})
    elseif(mp3lame_FOUND AND USE_VCPKG)
        make_plugin(mp3out mp3out.c mp3lame::mp3lame)
        target_include_directories(mp3out PRIVATE ${CSOUND_INCLUDE_DIR})
    endif()

    message(STATUS "Building MP3 opcodes")
endif()
