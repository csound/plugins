option(BUILD_WIIMOTE_OPCODES "Build the Wiimote opcodes" ON)

find_path(WIIUSE_INCLUDE_DIR wiiuse.h /usr/local/include)
find_library(WIIUSE_LIB wiiuse)
check_deps(BUILD_WIIMOTE_OPCODES WIIUSE_LIB)

if(BUILD_WIIMOTE_OPCODES)
    make_plugin(wiimote "6x" wiimote.c ${WIIUSE_LIB})
    target_include_directories(wiimote PRIVATE ${WIIUSE_INCLUDE_DIR} ${CSOUND_INCLUDE_DIR})
endif()
