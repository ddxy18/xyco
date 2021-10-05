include(GoogleTest)

file(GLOB_RECURSE xyco_test_src "*.cc")

set(CMAKE_CXX_FLAGS "-fprofile-instr-generate -fcoverage-mapping -DXYCO_TEST ${CMAKE_CXX_FLAGS}")

include_directories(.)

add_executable(xyco_test ${xyco_test_src} ${xyco_src_lib})
target_link_libraries(xyco_test gtest_main)
target_link_libraries(xyco_test GSL)
target_link_libraries(xyco_test spdlog)
gtest_discover_tests(xyco_test)