include(GoogleTest)

set(test_epoll_src "common/utils.cc" "fs/file.cc" "io/buffer.cc" "net/socket.cc" "net/tcp.cc" "runtime/future.cc" "runtime/join.cc" "runtime/select.cc" "sync/mpsc.cc" "sync/oneshot.cc" "time/sleep.cc" "time/timeout.cc" "utils/result_test.cc" "utils/epoll/fmt.cc")
set(test_uring_src "common/utils.cc" "fs/file.cc" "io/buffer.cc" "net/socket.cc" "net/tcp.cc" "runtime/future.cc" "runtime/join.cc" "runtime/select.cc" "sync/mpsc.cc" "sync/oneshot.cc" "time/sleep.cc" "time/timeout.cc" "utils/result_test.cc" "utils/io_uring/fmt.cc")

add_library(common_epoll ${test_epoll_src})
target_link_libraries(common_epoll fs_epoll io_epoll net_epoll runtime time gtest GSL)
add_library(common_uring ${test_uring_src})
target_link_libraries(common_uring fs_uring io_uring net_uring runtime time gtest GSL)

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping ${CMAKE_CXX_FLAGS}")

include_directories(common)

add_executable(xyco_test_epoll ${test_epoll_src})
target_link_libraries(xyco_test_epoll common_epoll gtest_main)
gtest_discover_tests(xyco_test_epoll)

add_executable(xyco_test_uring ${test_uring_src})
target_link_libraries(xyco_test_uring common_uring gtest_main)
gtest_discover_tests(xyco_test_uring)
