include_directories(.)

set(CMAKE_CXX_FLAGS "-DXYCO_ENABLE_LOG ${CMAKE_CXX_FLAGS}")

add_executable(xyco_benchmark_echo_server echo_server.cc ${xyco_src_lib})
target_link_libraries(xyco_benchmark_echo_server GSL)
target_link_libraries(xyco_benchmark_echo_server spdlog)
target_link_libraries(xyco_benchmark_echo_server uring)