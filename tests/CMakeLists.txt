include(GoogleTest)

file(GLOB xyco_test_src "fs/*.cc" "io/*.cc" "net/*.cc" "runtime/*.cc" "sync/*.cc" "time/*.cc" "utils/*.cc" "common/utils.cc")

file(GLOB xyco_test_uring_src "common/io_uring/*.cc" "utils/io_uring/*.cc")
file(GLOB xyco_test_epoll_src "common/epoll/*.cc" "utils/epoll/*.cc")

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping ${CMAKE_CXX_FLAGS}")

include_directories(.)

add_executable(xyco_test_uring ${xyco_test_uring_src} ${xyco_test_src} ${xyco_src_lib})
target_link_libraries(xyco_test_uring GSL)
target_link_libraries(xyco_test_uring spdlog)
target_link_libraries(xyco_test_uring uring)
target_link_libraries(xyco_test_uring gtest_main)
target_include_directories(xyco_test_uring PRIVATE common/io_uring)
gtest_discover_tests(xyco_test_uring)

add_executable(xyco_test_epoll ${xyco_test_epoll_src} ${xyco_test_src} ${xyco_src_lib})
target_link_libraries(xyco_test_epoll GSL)
target_link_libraries(xyco_test_epoll spdlog)
target_link_libraries(xyco_test_epoll uring)
target_link_libraries(xyco_test_epoll gtest_main)
target_include_directories(xyco_test_epoll PRIVATE common/epoll)
gtest_discover_tests(xyco_test_epoll)