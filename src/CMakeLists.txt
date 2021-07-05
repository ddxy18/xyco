file(
  GLOB
  xywebserver_src
  "*.cc"
  "event/net/*.cc"
  "event/runtime/*.cc"
  "event/io/*.cc"
  "utils/*.cc")

find_package(spdlog REQUIRED)

add_executable(xywebserver ${xywebserver_src})
target_link_libraries(xywebserver fmt::fmt)
target_link_libraries(xywebserver GSL)
target_link_libraries(xywebserver spdlog::spdlog)
