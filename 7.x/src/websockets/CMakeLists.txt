# Build the websocket opcode (websocket), which exposes Csound variables over a
# libwebsockets server.
option(BUILD_WEBSOCKET_OPCODE "Build the websocket opcode" ON)

if(USE_VCPKG)
    find_package(libwebsockets CONFIG)
    find_package(libuv CONFIG)
    set(WEBSOCKET_DEPS libwebsockets_FOUND libuv_FOUND)
else()
    find_path(WEBSOCKETS_H libwebsockets.h /usr/local/include /usr/include)
    find_library(websockets_library NAMES websockets)
    set(WEBSOCKET_DEPS websockets_library WEBSOCKETS_H)

    if(APPLE)
        find_library(ssl_library NAMES ssl)
        find_library(crypto_library NAMES crypto)
        list(APPEND WEBSOCKET_DEPS ssl_library crypto_library)
    endif()
endif()

check_deps(BUILD_WEBSOCKET_OPCODE ${WEBSOCKET_DEPS})

if(BUILD_WEBSOCKET_OPCODE)
    make_plugin(websocketIO WebSocketOpcode.c)
    target_include_directories(websocketIO PRIVATE ${CSOUND_INCLUDE_DIRS})
    target_compile_definitions(websocketIO PRIVATE BUILD_PLUGINS)

    if(USE_VCPKG)
        target_link_libraries(websocketIO websockets uv_a)
    else()
        target_link_libraries(websocketIO ${websockets_library})
        target_include_directories(websocketIO PRIVATE ${WEBSOCKETS_H})
    endif()

    if(APPLE)
        if(USE_VCPKG)
            target_link_libraries(websocketIO z)
        else()
            target_link_libraries(websocketIO ${ssl_library} ${crypto_library} z)
        endif()
    endif()
endif()
