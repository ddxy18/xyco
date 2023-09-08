include(GoogleTest)

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping"
                    ${CMAKE_CXX_FLAGS})

add_library(common_epoll common/utils.cc)
target_sources(common_epoll PUBLIC common/utils.h)
target_link_libraries(
  common_epoll
  PUBLIC runtime
         io_epoll
         fs_epoll
         net_epoll
         task
         time
         gtest)
target_include_directories(common_epoll PUBLIC common)

add_library(common_uring common/utils.cc)
target_sources(common_uring PUBLIC common/utils.h)
target_link_libraries(
  common_uring
  PUBLIC runtime
         io_uring
         fs_uring
         net_uring
         task
         time
         gtest)
target_include_directories(common_uring PUBLIC common)

add_executable(
  xyco_test_epoll
  main.cc
  fs/file.cc
  io/buffer.cc
  net/socket.cc
  net/tcp.cc
  runtime/future.cc
  runtime/join.cc
  runtime/runtime.cc
  runtime/select.cc
  sync/mpsc.cc
  sync/oneshot.cc
  time/clock.cc
  time/sleep.cc
  time/timeout.cc
  utils/epoll/fmt_test.cc
  utils/fmt_test.cc)
target_link_libraries(xyco_test_epoll PRIVATE common_epoll)

add_executable(
  xyco_test_uring
  main.cc
  fs/file.cc
  io/buffer.cc
  net/socket.cc
  net/tcp.cc
  runtime/future.cc
  runtime/join.cc
  runtime/runtime.cc
  runtime/select.cc
  sync/mpsc.cc
  sync/oneshot.cc
  time/clock.cc
  time/sleep.cc
  time/timeout.cc
  utils/io_uring/fmt_test.cc
  utils/fmt_test.cc)
target_link_libraries(xyco_test_uring PRIVATE common_uring)
