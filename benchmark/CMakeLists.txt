include_directories(.)

add_executable(xyco_benchmark_echo_server echo_server.cc ${xyco_src_lib})
target_link_libraries(xyco_benchmark_echo_server GSL)
target_link_libraries(xyco_benchmark_echo_server spdlog)