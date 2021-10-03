include(GoogleTest)

file(GLOB xywebserver_test_src "*.cc" "utils/*.cc" "event/*.cc" "event/net/*.cc")
file(GLOB xywebserver_src_lib "../src/event/net/*.cc"
     "../src/event/runtime/*.cc" "../src/event/io/*.cc" "../src/utils/*.cc")

set(CMAKE_CXX_FLAGS "-DXYWEBSERVER_TEST ${CMAKE_CXX_FLAGS}")

add_executable(xywebserver_test ${xywebserver_test_src} ${xywebserver_src_lib})
target_link_libraries(xywebserver_test gtest_main)
target_link_libraries(xywebserver_test GSL)
target_link_libraries(xywebserver_test spdlog)
gtest_discover_tests(xywebserver_test)
