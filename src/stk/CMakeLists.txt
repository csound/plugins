option(BUILD_STK_OPCODES "Build the stk opcodes" ON)
option(STK_EMBED_RAWWAVES "Embed STK's raw-wave files in the plugin" ${WASM})

# The STK sources are bundled under this directory (include/, src/ and
# rawwaves/) so that a patched copy is always built, independent of any system
# or package-manager STK installation. In particular, src/Sitar.cpp carries a
# fix for the sample buildup described in csound/plugins#4.
if(BUILD_STK_OPCODES)
    file(GLOB stk_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
    list(APPEND stk_srcs "${CMAKE_CURRENT_SOURCE_DIR}/stkOpcodes.cpp")

    if(WASM)
        if(NOT WASM_LIBCXX_EXCEPTIONS OR NOT WASM_CXX_EXCEPTIONS_INCLUDE_DIR OR NOT WASM_LIBUNWIND)
            message(FATAL_ERROR "STK needs the WASM C++ exception runtime and unwinder")
        endif()
        set(WASM_CXX_EXCEPTIONS ON)
        set(WASM_LIBCXX "${WASM_LIBCXX_EXCEPTIONS}")
        set(WASM_CXX_INCLUDE_DIR "${WASM_CXX_EXCEPTIONS_INCLUDE_DIR}")
    endif()
    make_plugin(stkops "${stk_srcs}")
    if(WASM)
        target_link_libraries(stkops PRIVATE "${WASM_LIBUNWIND}")
    endif()
    target_include_directories(stkops PRIVATE
        ${CSOUND_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(stkops PRIVATE BUILD_PLUGINS)

    if(STK_EMBED_RAWWAVES)
        file(GLOB rawwaves CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rawwaves/*.raw")
        set(embedded_waves "${CMAKE_CURRENT_BINARY_DIR}/stk_rawwaves.inc")
        add_custom_command(OUTPUT "${embedded_waves}"
            COMMAND ${CMAKE_COMMAND}
                "-DRAWWAVE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/rawwaves"
                "-DOUTPUT_FILE=${embedded_waves}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/EmbedRawwaves.cmake"
            DEPENDS ${rawwaves} "${CMAKE_CURRENT_SOURCE_DIR}/EmbedRawwaves.cmake"
            VERBATIM)
        target_sources(stkops PRIVATE "${embedded_waves}")
        target_include_directories(stkops PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
        target_compile_definitions(stkops PRIVATE STK_EMBED_RAWWAVES)
    endif()

    # Install the rawwaves used by the STK instruments and make them the
    # default raw wave path; the RAWWAVE_PATH environment variable overrides it.
    if(NOT STK_EMBED_RAWWAVES)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rawwaves"
                DESTINATION "${PLUGIN_INSTALL_DIR}/stk")
        if(PLUGIN_INSTALL_DIR)
            target_compile_definitions(stkops PRIVATE
                DEFAULT_RAWWAVE_PATH="${PLUGIN_INSTALL_DIR}/stk/rawwaves")
        endif()
    endif()

    # Native unit tests exercise both adapters without an audio device or engine.
    if(NOT WASM)
        find_package(GTest QUIET)
        if(GTest_FOUND)
            add_executable(stk_adapter_tests tests/stk_adapter_test.cpp src/Stk.cpp)
            set_target_properties(stk_adapter_tests PROPERTIES
                CXX_STANDARD 14
                CXX_STANDARD_REQUIRED ON)
            target_include_directories(stk_adapter_tests PRIVATE
                ${CSOUND_INCLUDE_DIRS}
                "${CMAKE_CURRENT_SOURCE_DIR}"
                "${CMAKE_CURRENT_SOURCE_DIR}/include")
            target_compile_definitions(stk_adapter_tests PRIVATE BUILD_PLUGINS)
            target_link_libraries(stk_adapter_tests PRIVATE GTest::gtest_main)
            add_test(NAME stk_adapter_tests COMMAND stk_adapter_tests)
        endif()
    endif()

    message(STATUS "Building STK opcodes")
endif()
