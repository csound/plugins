option(BUILD_MP3OUT_OPCODE "Build the mp3out opcode" ON)
find_package(LAME)

check_deps(BUILD_MP3OUT_OPCODE LAME_FOUND)
if(BUILD_MP3OUT_OPCODE)
    make_plugin(mp3out lame.c ${LAME_LIBRARY})
    target_include_directories(mp3out PRIVATE ${LAME_INCLUDE_DIR})
    target_include_directories(mp3out PRIVATE ${CSOUND_INCLUDE_DIR})
endif()
