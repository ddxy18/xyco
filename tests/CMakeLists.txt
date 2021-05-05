include(GoogleTest)

add_executable(xywebserver_test hello_test.cc)
target_link_libraries(xywebserver_test gtest_main)
gtest_discover_tests(xywebserver_test)
