file(GLOB_RECURSE xywebserver_src "*.cc")

add_executable(xywebserver ${xywebserver_src_lib} main.cc)
target_link_libraries(xywebserver GSL)
target_link_libraries(xywebserver spdlog)