add_executable(xyco_http_server http_server.cc)
target_link_libraries(xyco_http_server PRIVATE xyco::fs xyco::io xyco::net
                                               xyco::task xyco::runtime)
