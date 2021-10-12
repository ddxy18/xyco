add_executable(xyco ${xyco_src_lib} main.cc)
target_link_libraries(xyco GSL)
target_link_libraries(xyco spdlog)