CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(beer)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


if(EXISTS "/usr/include/wiringPi.h")
  message("found wiringPi!")
  add_definitions(-DHAVE_WIRINGPI=1)
  set(wiringPi-lib "wiringPi")
else()
  message("did not find wiringPi lib")
  add_definitions(-DHAVE_WIRINGPI=0)
  set(wiringPi-lib "")
endif()

if(NOT EXISTS "/var/log/beer")
  add_custom_target(
    make_var_log_dir ALL
    COMMAND sudo mkdir -p /var/log/beer
    COMMAND sudo chmod a+rw /var/log/beer
    COMMENT "making /var/log/beer directory"
    )
else()
  add_custom_target(
    make_var_log_dir ALL
    COMMENT "/var/log/beer directory exists"
    )
endif()

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

find_package(Check)
if(CHECK_FOUND)
  message(STATUS "Check library found.  Run all tests by  issuing `make test`")
  include(CheckCSourceCompiles)
  include(CheckCSourceRuns)
  include(CheckFunctionExists)
  include(CheckIncludeFile)
  include(CheckIncludeFiles)
  include(CheckLibraryExists)
  include(CheckSymbolExists)
  include(CheckTypeSize)

  include_directories(${CHECK_INCLUDE_DIRS})
  link_directories(${CHECK_LIBRARY_DIRS})

  add_subdirectory(test)

  enable_testing()
  add_test(NAME check_dummy COMMAND check_dummy)

else()

  message(STATUS "WARNING. Check library not found. Test suite will not be  built.")

endif()
