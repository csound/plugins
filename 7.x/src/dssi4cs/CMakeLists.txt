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
endif()
