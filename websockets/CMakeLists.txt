option(BUILD_WEBSOCKET_OPCODE "Build the websocket opcode" ON)
FIND_PATH (WEBSOCKETS_H libwebsockets.h
    /usr/local/include
    /usr/include
)

if(WIN32 AND NOT MSVC)
  find_library (websockets_library NAMES websockets_static)
else()
  FIND_LIBRARY (websockets_library websockets
      HINTS
      /usr/local/lib
      /usr/lib
  )
  find_library (websockets_library NAMES websockets)
  if(APPLE)
    FIND_LIBRARY (ssl_library NAMES ssl
      HINTS
      /usr/local/lib
      /usr/lib )
   FIND_LIBRARY (crypto_library NAMES crypto
      HINTS
      /usr/local/lib
      /usr/lib )
 endif()
endif()
check_include_file(libwebsockets.h WEBSOCKETS_H)
if(APPLE)
  check_deps(BUILD_WEBSOCKET_OPCODE websockets_library ssl_library
     crypto_library WEBSOCKETS_H)
else()
  check_deps(BUILD_WEBSOCKET_OPCODE websockets_library WEBSOCKETS_H)
endif()

if(BUILD_WEBSOCKET_OPCODE)
 	make_plugin(websocketIO WebSocketOpcode.c)
 	target_link_libraries(websocketIO ${websockets_library})
        message(STATUS "WEBSOCKETS ${websockets_library}")
        target_include_directories(websocketIO PRIVATE ${CSOUND_INCLUDE_DIRS})
        target_include_directories(websocketIO PRIVATE ${WEBSOCKETS_H})
 if(APPLE)
   target_link_libraries(websocketIO /usr/local/lib/libssl.a /usr/local/lib/libcrypto.a  z)
 endif()
 if(WIN32 AND NOT MSVC)
    target_link_libraries(websocketIO ssl crypto z ws2_32 wsock32)
  endif()
endif()
