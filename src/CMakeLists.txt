file(
  GLOB
  xywebserver_src
  "*.cc"
  "event/net/*.cc"
  "event/runtime/*.cc"
  "event/io/*.cc"
  "utils/*.cc")


add_executable(xywebserver ${xywebserver_src})
target_link_libraries(xywebserver GSL)
target_link_libraries(xywebserver spdlog)
