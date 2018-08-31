CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(beer)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_custom_target(
  make_var_log_dir ALL
  COMMAND sudo mkdir -p /var/log/beer
  COMMAND sudo chmod a+rw /var/log/beer
  COMMENT "making /var/log/beer directory"
  )

include_directories(
  src
  )

add_library(beer_server-lib
  src/beer_server.c
  src/beer_utils.c
  )
target_link_libraries(beer_server-lib
  pthread m
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

