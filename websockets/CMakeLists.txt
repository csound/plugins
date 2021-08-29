option(BUILD_WEBSOCKET_OPCODE "Build the websocket opcode" ON)
FIND_PATH (WEBSOCKETS_H libwebsockets.h /usr/local/include /usr/include)

FIND_LIBRARY(websockets_library NAMES websockets)

if(APPLE)
  FIND_LIBRARY (ssl_library NAMES ssl)
  FIND_LIBRARY (crypto_library NAMES crypto)
  check_deps(BUILD_WEBSOCKET_OPCODE websockets_library ssl_library
     crypto_library WEBSOCKETS_H)
else()
  check_deps(BUILD_WEBSOCKET_OPCODE websockets_library WEBSOCKETS_H)
endif()

if(BUILD_WEBSOCKET_OPCODE)
 	make_plugin(websocketIO WebSocketOpcode.c)
 	target_link_libraries(websocketIO ${websockets_library})
        message(STATUS "WEBSOCKETS LIB: ${websockets_library}")
        target_include_directories(websocketIO PRIVATE ${CSOUND_INCLUDE_DIRS})
        target_include_directories(websocketIO PRIVATE ${WEBSOCKETS_H})
       if(APPLE)
         target_link_libraries(websocketIO ${ssl_library} ${crypto_library}  z)
       endif()
endif()
