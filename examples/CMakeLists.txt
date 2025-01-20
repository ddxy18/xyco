add_executable(xyco_epoll_http_server http_server.cc)
target_link_libraries(
  xyco_epoll_http_server PRIVATE xyco::fs_epoll xyco::io_epoll xyco::net_epoll
                                 xyco::task xyco::runtime)

add_executable(xyco_uring_http_server http_server.cc)
target_link_libraries(
  xyco_uring_http_server PRIVATE xyco::fs_uring xyco::io_uring xyco::net_uring
                                 xyco::task xyco::runtime)
