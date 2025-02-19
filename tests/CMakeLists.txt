include(GoogleTest)

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping"
                    ${CMAKE_CXX_FLAGS})

add_library(xyco_test_utils common/utils.cc)
target_sources(xyco_test_utils PUBLIC FILE_SET utils_module TYPE CXX_MODULES
                                      FILES common/utils.ccm)
target_include_directories(xyco_test_utils PUBLIC common)
target_link_libraries(
  xyco_test_utils
  PUBLIC xyco::runtime
         xyco::sync
         xyco::task
         xyco::time
         xyco::fs
         xyco::net
         xyco::io
         gtest)

add_executable(
  xyco_test
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
  utils/${XYCO_IO_API}/fmt_test.cc
  utils/fmt_test.cc)
target_link_libraries(xyco_test PRIVATE xyco_test_utils)
