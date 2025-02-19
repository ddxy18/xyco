add_executable(xyco_echo_server echo_server.cc)
target_link_libraries(xyco_echo_server PRIVATE xyco::io xyco::net xyco::task
                                               xyco::runtime)

add_executable(asio_echo_server asio_echo_server.cc)
target_compile_definitions(asio_echo_server PUBLIC ASIO_HAS_CO_AWAIT=1
                                                   ASIO_HAS_STD_COROUTINE=1)

if(XYCO_ENABLE_LINTING)
  target_compile_options(
    asio_echo_server
    PRIVATE -Wno-deprecated-copy-with-user-provided-dtor
            -Wno-deprecated-literal-operator
            -Wno-deprecated-redundant-constexpr-static-def -Wno-unknown-pragmas)
endif(XYCO_ENABLE_LINTING)
