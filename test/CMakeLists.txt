add_executable(check_dummy check_dummy.c)
target_link_libraries(check_dummy beer_server-lib ${CHECK_LIBRARIES})
