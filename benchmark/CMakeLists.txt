add_executable(xyco_epoll_echo_server echo_server.cc ${xyco_src_lib})
target_link_libraries(xyco_epoll_echo_server GSL)
target_link_libraries(xyco_epoll_echo_server spdlog)
target_link_libraries(xyco_epoll_echo_server uring)
target_include_directories(xyco_epoll_echo_server PRIVATE ../src/io/standalone/epoll PRIVATE ../src/net/standalone/epoll)

add_executable(xyco_uring_echo_server echo_server.cc ${xyco_src_lib})
target_link_libraries(xyco_uring_echo_server GSL)
target_link_libraries(xyco_uring_echo_server spdlog)
target_link_libraries(xyco_uring_echo_server uring)
target_include_directories(xyco_uring_echo_server PRIVATE ../src/io/standalone/io_uring PRIVATE ../src/net/standalone/io_uring)

add_executable(xyco_asio_echo_server asio_echo_server.cc)
target_include_directories(xyco_asio_echo_server PUBLIC ../build/_deps/asio-src/asio/include)
