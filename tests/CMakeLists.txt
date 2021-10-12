include(GoogleTest)

file(GLOB_RECURSE xywebserver_test_src "*.cc")

set(CMAKE_CXX_FLAGS "-DXYWEBSERVER_TEST ${CMAKE_CXX_FLAGS}")

include_directories(.)

add_executable(xywebserver_test ${xywebserver_test_src} ${xywebserver_src_lib})
target_link_libraries(xywebserver_test gtest_main)
target_link_libraries(xywebserver_test GSL)
target_link_libraries(xywebserver_test spdlog)
gtest_discover_tests(xywebserver_test)