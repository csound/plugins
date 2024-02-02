option(BUILD_WEBSOCKET_OPCODE "Build the websocket opcode" ON)

if(USE_VCPKG)
    find_package(libuv CONFIG REQUIRED)
    find_package(libwebsockets CONFIG REQUIRED)

    if(NOT TARGET libwebsockets AND NOT TARGET libuv)
        message(STATUS "Could not find libwebsockets or libuv")
        set(BUILD_WEBSOCKET_OPCODE OFF)
    endif()
elseif(APPLE)
    find_library(ssl_library NAMES ssl)
    find_library(crypto_library NAMES crypto)

    check_deps(BUILD_WEBSOCKET_OPCODE ssl_library crypto_library)
else()
    find_path(WEBSOCKETS_H libwebsockets.h /usr/local/include /usr/include)
    find_library(websockets_library NAMES websockets)

    check_deps(BUILD_WEBSOCKET_OPCODE WEBSOCKETS_H websockets_library)
endif()

if(BUILD_WEBSOCKET_OPCODE)
 	make_plugin(websocketIO WebSocketOpcode.c)

    if(USE_VCPKG)
        target_link_libraries(websocketIO
            libwebsockets
            $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
        )
    else()
        target_link_libraries(websocketIO ${websockets_library})
        target_include_directories(websocketIO PRIVATE ${WEBSOCKETS_H})
    endif()

    if(APPLE)
        target_link_libraries(websocketIO ${ssl_library} ${crypto_library} z)
    endif()
endif()
