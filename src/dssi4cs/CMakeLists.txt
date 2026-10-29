# Build the DSSI/LADSPA hosting opcodes.
#
# Hosting native DSSI synths and LADSPA effects is Linux-only: the opcodes load
# plugin libraries with dlopen and exchange ALSA sequencer events, so they need
# the system ALSA headers (no ALSA library, and no running sequencer, is
# required). The DSSI and LADSPA API headers are vendored under ./include.
#
# The plugin is built by default and silently disabled where the platform is
# not Linux or the ALSA headers are missing.
option(BUILD_DSSI_OPCODES "Build the DSSI/LADSPA opcodes" ON)

find_path(DSSI_ALSA_ASOUNDLIB_H alsa/asoundlib.h /usr/local/include /usr/include)
find_path(DSSI_ALSA_SEQ_EVENT_H alsa/seq_event.h /usr/local/include /usr/include)

check_deps(BUILD_DSSI_OPCODES LINUX DSSI_ALSA_ASOUNDLIB_H DSSI_ALSA_SEQ_EVENT_H)

if(BUILD_DSSI_OPCODES)
    make_plugin(dssi4cs "load.c;dssi4cs.c" ${CMAKE_DL_LIBS} m)
    target_include_directories(dssi4cs PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/dssi
        ${CMAKE_CURRENT_SOURCE_DIR}/include/ladspa
        ${DSSI_ALSA_ASOUNDLIB_H}
        ${CSOUND_INCLUDE_DIRS})
    target_compile_definitions(dssi4cs PRIVATE BUILD_PLUGINS)

    # On multiarch distributions (Debian, Ubuntu, ...) plugins may be
    # installed under /usr/lib/<triplet>/dssi and /usr/lib/<triplet>/ladspa.
    # Prepend the triplet detected for this build to the default search path;
    # DSSI_PATH and LADSPA_PATH still take precedence at runtime.
    if(CMAKE_LIBRARY_ARCHITECTURE)
        target_compile_definitions(dssi4cs PRIVATE
            "DSSI4CS_DEFAULT_PATH=\"/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dssi:/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/ladspa:/usr/local/lib/dssi:/usr/lib/dssi:/usr/lib64/dssi:/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/lib64/ladspa\"")
    endif()

    # Regression tests for the host, built and registered with CTest when
    # GoogleTest is available. The fixture is a small real DSSI/LADSPA plugin
    # built from the vendored headers, so its ABI matches the host.
    find_package(GTest QUIET)
    if(GTest_FOUND)
        add_library(dssi_test_plugin MODULE tests/fixtures/dssi_test_plugin.c)
        add_library(ladspa_test_plugin MODULE tests/fixtures/dssi_test_plugin.c)
        target_compile_definitions(ladspa_test_plugin PRIVATE DSSI_TEST_LADSPA)
        foreach(plugin dssi_test_plugin ladspa_test_plugin)
            target_include_directories(${plugin} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include/dssi
                ${CMAKE_CURRENT_SOURCE_DIR}/include/ladspa
                ${DSSI_ALSA_ASOUNDLIB_H})
        endforeach()

        add_executable(dssi_tests tests/dssi_test.cpp)
        # Recent GoogleTest releases require C++14, which is newer than the
        # C++11 default used for the plugin itself.
        set_target_properties(dssi_tests PROPERTIES
            CXX_STANDARD 14
            CXX_STANDARD_REQUIRED ON)
        target_include_directories(dssi_tests PRIVATE ${CSOUND_INCLUDE_DIRS})
        target_link_libraries(dssi_tests PRIVATE
            GTest::gtest_main ${CSOUND_LIBRARY} ${CMAKE_DL_LIBS})
        target_compile_definitions(dssi_tests PRIVATE
            CSOUND_TEST_DSSI_HOST="$<TARGET_FILE:dssi4cs>"
            CSOUND_TEST_DSSI_PLUGIN="$<TARGET_FILE:dssi_test_plugin>"
            CSOUND_TEST_LADSPA_PLUGIN="$<TARGET_FILE:ladspa_test_plugin>")
        add_dependencies(dssi_tests dssi4cs dssi_test_plugin ladspa_test_plugin)
        add_test(NAME dssi_tests COMMAND dssi_tests)
    endif()
endif()
