option(BUILD_JACK_OPCODES "Build the JACK related opcodes" ON)

find_library(JACK_LIB NAMES jack)
find_path (JACK_INCLUDE_DIR jack/jack.h HINTS /usr/local/include)

check_deps(BUILD_JACK_OPCODES JACK_LIB)

if(BUILD_JACK_OPCODES AND JACK_LIB)
    list(APPEND jack_LIBS ${JACK_LIB})

    make_plugin(jacko jacko.cpp "${jack_LIBS}")
    target_include_directories(jacko PRIVATE ${JACK_INCLUDE_DIR})
    set_property(TARGET jacko PROPERTY CXX_STANDARD 20)
    set_property(TARGET jacko PROPERTY CXX_STANDARD_REQUIRED On)
    set_property(TARGET jacko PROPERTY CXX_EXTENSIONS Off)

    make_plugin(jackTransport jackTransport.c "${jack_LIBS}")
    target_include_directories(jackTransport PRIVATE ${JACK_INCLUDE_DIR})
endif()
