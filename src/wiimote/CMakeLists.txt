# Build the wiimote opcodes (requires the wiiuse library)
option(BUILD_WIIMOTE_OPCODES "Build the Wiimote opcodes" ON)

find_path(WIIUSE_INCLUDE_DIR wiiuse.h /usr/local/include /usr/include)
find_library(WIIUSE_LIB wiiuse)
check_deps(BUILD_WIIMOTE_OPCODES WIIUSE_INCLUDE_DIR WIIUSE_LIB)

if(BUILD_WIIMOTE_OPCODES)
    make_plugin(wiimote wiimote.c ${WIIUSE_LIB})
    target_include_directories(wiimote PRIVATE ${WIIUSE_INCLUDE_DIR} ${CSOUND_INCLUDE_DIRS})
    target_compile_definitions(wiimote PRIVATE BUILD_PLUGINS)
endif()
