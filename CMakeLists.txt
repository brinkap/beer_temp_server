CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(beer)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


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
  /usr/local
  /usr/local/include
  )

find_library(mosquitto-lib
  NAME libmosquitto.so
  PATHS /usr/local/lib
  )
message(status, "mosquitto lib is ${mosquitto-lib}")


add_library(beer_server-lib
  src/beer_server.c
  src/beer_utils.c
  )
target_link_libraries(beer_server-lib
  pthread m 
  ${wiringPi-lib}
  ${mosquitto-lib}
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
