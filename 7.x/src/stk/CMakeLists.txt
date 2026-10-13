option(BUILD_STK_OPCODES "Build the stk opcodes" ON)

# The STK sources are bundled under this directory (include/, src/ and
# rawwaves/) so that a patched copy is always built, independent of any system
# or package-manager STK installation. In particular, src/Sitar.cpp carries a
# fix for the sample buildup described in csound/plugins#4.
if(BUILD_STK_OPCODES)
    file(GLOB stk_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
    list(APPEND stk_srcs "${CMAKE_CURRENT_SOURCE_DIR}/stkOpcodes.cpp")

    make_plugin(stkops "${stk_srcs}")
    target_include_directories(stkops PRIVATE
        ${CSOUND_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(stkops PRIVATE BUILD_PLUGINS)

    # Install the rawwaves used by the STK instruments and make them the
    # default raw wave path; the RAWWAVE_PATH environment variable overrides it.
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rawwaves"
            DESTINATION "${PLUGIN_INSTALL_DIR}/stk")
    if(PLUGIN_INSTALL_DIR)
        target_compile_definitions(stkops PRIVATE
            DEFAULT_RAWWAVE_PATH="${PLUGIN_INSTALL_DIR}/stk/rawwaves")
    endif()

    message(STATUS "Building STK opcodes")
endif()
