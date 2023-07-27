add_executable(xyco_epoll_http_server http_server.cc)
target_link_libraries(xyco_epoll_http_server PRIVATE fs_epoll io_epoll
                                                     net_epoll task runtime)

add_executable(xyco_uring_http_server http_server.cc)
target_link_libraries(xyco_uring_http_server PRIVATE fs_uring io_uring
                                                     net_uring task runtime)
