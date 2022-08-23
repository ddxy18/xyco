add_executable(xyco_benchmark_echo_server echo_server.cc ${xyco_src_lib})
target_link_libraries(xyco_benchmark_echo_server GSL)
target_link_libraries(xyco_benchmark_echo_server spdlog)
target_link_libraries(xyco_benchmark_echo_server uring)

add_executable(xyco_asio_echo_server asio_echo_server.cc)
target_include_directories(xyco_asio_echo_server PUBLIC ../build/_deps/asio-src/asio/include)
