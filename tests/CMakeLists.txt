include(GoogleTest)

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping"
                    ${CMAKE_CXX_FLAGS})

add_library(xyco_test_common_utils)
target_sources(xyco_test_common_utils PUBLIC FILE_SET common_utils_module TYPE
                                             CXX_MODULES FILES common/utils.ccm)
target_include_directories(xyco_test_common_utils PUBLIC common)
target_link_libraries(xyco_test_common_utils PUBLIC xyco::runtime xyco::task
                                                    xyco::time xyco::sync gtest)

add_library(xyco_test_epoll_utils common/utils.cc)
target_link_libraries(
  xyco_test_epoll_utils PUBLIC xyco_test_common_utils xyco::io_epoll
                               xyco::fs_epoll xyco::net_epoll)
add_library(xyco_test_uring_utils common/utils.cc)
target_link_libraries(
  xyco_test_uring_utils PUBLIC xyco_test_common_utils xyco::io_uring
                               xyco::fs_uring xyco::net_uring)

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
  task/blocking_task.cc
  time/clock.cc
  time/sleep.cc
  time/timeout.cc
  utils/epoll/fmt_test.cc
  utils/fmt_test.cc)
target_link_libraries(xyco_test_epoll PRIVATE xyco_test_epoll_utils)
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
  task/blocking_task.cc
  time/clock.cc
  time/sleep.cc
  time/timeout.cc
  utils/io_uring/fmt_test.cc
  utils/fmt_test.cc)
target_link_libraries(xyco_test_uring PRIVATE xyco_test_uring_utils)
