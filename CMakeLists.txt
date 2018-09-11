CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(beer)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
SET(CMAKE_BUILD_TYPE Debug)


if(EXISTS "/usr/include/wiringPi.h")
  message("found wiringPi!")
  add_definitions(-DHAVE_WIRINGPI=1)
  set(wiringPi-lib "wiringPi")
else()
  message("did not find wiringPi lib")
  add_definitions(-DHAVE_WIRINGPI=0)
  set(wiringPi-lib "")
endif()

add_custom_target(
  make_var_log_dir ALL
  COMMAND sudo mkdir -p /var/log/beer
  COMMAND sudo chmod a+rw /var/log/beer
  COMMENT "making /var/log/beer directory"
  )

include_directories(
  src
  /usr/local/include
  /usr/local/include/libwebsockets
  )

add_library(beer_server-lib
  src/beer_server.c
  src/beer_utils.c
  src/base64.c
  )
target_link_libraries(beer_server-lib
  pthread m ${wiringPi-lib}
  crypto
  )

add_executable(run_beer_server
  src/run_beer_server.c
  )
add_dependencies(run_beer_server 
  make_var_log_dir
  )

target_link_libraries(run_beer_server
  beer_server-lib
  )


add_executable(client_simple_test
  src/client_simple_test.c)

add_executable(ws_handshake_test
  src/ws_handshake_test.c
  src/base64.c
  )
target_link_libraries(ws_handshake_test
  crypto
  )
find_library(websockets-lib
  NAMES libwebsockets.a
  PATHS /usr/local/lib
  )
message(status, "Websockets-lib ${websockets-lib}")

add_executable(libwebsockets_simple_server
  src/libwebsockets_simple_server.c
  )
target_link_libraries(libwebsockets_simple_server
  ${websockets-lib}
  pthread
  crypto
  ssl
  )

add_executable(simple_server
  src/simple_server.c
  )
target_link_libraries(simple_server
  ${websockets-lib}
  pthread
  crypto
  ssl
  )

