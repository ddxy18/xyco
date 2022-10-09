set(CMAKE_CXX_FLAGS "-DXYCO_ENABLE_LOG ${CMAKE_CXX_FLAGS}")

add_executable(xyco_epoll_main ${xyco_src_lib} main.cc)
target_link_libraries(xyco_epoll_main GSL)
target_link_libraries(xyco_epoll_main spdlog)
target_link_libraries(xyco_epoll_main uring)
target_include_directories(xyco_epoll_main PRIVATE io/standalone/epoll PRIVATE net/standalone/epoll)

add_executable(xyco_uring_main ${xyco_src_lib} main.cc)
target_link_libraries(xyco_uring_main GSL)
target_link_libraries(xyco_uring_main spdlog)
target_link_libraries(xyco_uring_main uring)
target_include_directories(xyco_uring_main PRIVATE io/standalone/io_uring PRIVATE net/standalone/io_uring)
