add_executable(xyco_epoll_echo_server echo_server.cc)
target_link_libraries(xyco_epoll_echo_server PRIVATE io_epoll net_epoll task
                                                     runtime)

add_executable(xyco_uring_echo_server echo_server.cc)
target_link_libraries(xyco_uring_echo_server PRIVATE io_uring net_uring task
                                                     runtime)

add_executable(asio_echo_server asio_echo_server.cc)
target_include_directories(asio_echo_server
                           PUBLIC ../build/_deps/asio-src/asio/include)
target_link_libraries(asio_echo_server GSL)
