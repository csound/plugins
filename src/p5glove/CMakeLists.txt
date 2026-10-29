# Build the P5Glove opcodes.
#
# The P5Glove driver is not available as a standard distribution package, and
# the vcpkg registry has neither libp5glove nor the libusb-0.1 compatibility
# layer its Linux backend needs, so this plugin is disabled in the automated
# CI/release builds (see the README for details).
#
# On macOS the driver can additionally be built from the bundled sources under
# ./libp5glove by setting P5GLOVE_USE_BUNDLED=ON.
option(BUILD_P5GLOVE_OPCODES "Build the P5Glove opcodes" ON)
option(P5GLOVE_USE_BUNDLED "Build p5glove from the bundled libp5glove sources (macOS)" OFF)

if(APPLE AND P5GLOVE_USE_BUNDLED)
    if(BUILD_P5GLOVE_OPCODES)
        make_plugin(p5g "p5glove.c;libp5glove/p5glove.c;libp5glove/usb_hid_macosx.c")
        target_link_libraries(p5g PRIVATE "-framework IOKit" "-framework CoreFoundation")
        target_include_directories(p5g PRIVATE ./libp5glove ${CSOUND_INCLUDE_DIRS})
        target_compile_definitions(p5g PRIVATE BUILD_PLUGINS)
    endif()
else()
    find_path(P5GLOVE_INCLUDE_DIR p5glove.h /usr/local/include /usr/include)
    find_library(P5GLOVE_LIB p5glove)
    check_deps(BUILD_P5GLOVE_OPCODES P5GLOVE_INCLUDE_DIR P5GLOVE_LIB)

    if(BUILD_P5GLOVE_OPCODES)
        make_plugin(p5g p5glove.c ${P5GLOVE_LIB})
        target_include_directories(p5g PRIVATE ${P5GLOVE_INCLUDE_DIR} ${CSOUND_INCLUDE_DIRS})
        target_compile_definitions(p5g PRIVATE BUILD_PLUGINS)
    endif()
endif()
