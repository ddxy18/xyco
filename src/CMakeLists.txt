add_executable(xyco_epoll_main main.cc)
target_link_libraries(xyco_epoll_main io_epoll net_epoll runtime)

add_executable(xyco_uring_main main.cc)
target_link_libraries(xyco_uring_main io_uring net_uring runtime)

set(fs_epoll_src "fs/utils.cc" "fs/epoll/file.cc")
set(fs_uring_src "fs/utils.cc" "fs/io_uring/file.cc")
set(fs_all_src "fs/utils.cc" "fs/epoll/file.cc" "fs/io_uring/file.cc")

set(io_epoll_src "io/epoll/registry.cc" "io/epoll/extra.cc")
set(io_uring_src "io/io_uring/registry.cc" "io/io_uring/extra.cc")
set(io_all_src "io/epoll/registry.cc" "io/epoll/extra.cc" "io/io_uring/registry.cc" "io/io_uring/extra.cc")

set(net_epoll_src "net/socket.cc" "net/epoll/listener.cc")
set(net_uring_src "net/socket.cc" "net/io_uring/listener.cc")
set(net_all_src "net/socket.cc" "net/epoll/listener.cc" "net/io_uring/listener.cc")

set(runtime_src "runtime/blocking.cc" "runtime/driver.cc" "runtime/future.cc" "runtime/runtime.cc")

set(time_src "time/driver.cc" "time/wheel.cc" "time/clock.cc")

set(utils_src "utils/error.cc" "utils/panic.cc")

add_library(fs_epoll ${fs_epoll_src})
target_link_libraries(fs_epoll io_epoll runtime utils)
target_include_directories(fs_epoll INTERFACE fs/standalone/epoll)
add_library(fs_uring ${fs_uring_src})
target_link_libraries(fs_uring io_uring runtime utils uring)
target_include_directories(fs_uring INTERFACE fs/standalone/io_uring)
add_library(fs_all ${fs_all_src})
target_link_libraries(fs_all io_all runtime utils uring)

add_library(io_epoll ${io_epoll_src})
target_link_libraries(io_epoll runtime utils spdlog)
target_include_directories(io_epoll INTERFACE io/standalone/epoll)
add_library(io_uring ${io_uring_src})
target_link_libraries(io_uring runtime utils spdlog uring)
target_include_directories(io_uring INTERFACE io/standalone/io_uring)
add_library(io_all ${io_all_src})
target_link_libraries(io_all runtime utils spdlog uring)

add_library(net_epoll ${net_epoll_src})
target_link_libraries(net_epoll io_epoll runtime utils spdlog)
target_include_directories(net_epoll INTERFACE net/standalone/epoll)
add_library(net_uring ${net_uring_src})
target_link_libraries(net_uring io_uring runtime utils spdlog uring)
target_include_directories(net_uring INTERFACE net/standalone/io_uring)
add_library(net_all ${net_all_src})
target_link_libraries(net_all io_all runtime utils spdlog uring)

add_library(runtime ${runtime_src})
target_link_libraries(runtime utils GSL)

add_library(time ${time_src})
target_link_libraries(time runtime spdlog)

option(ENABLE_LOGGING "OFF")

add_library(utils ${utils_src})
target_link_libraries(utils spdlog ${Boost_STACKTRACE_ADDR2LINE_LIBRARY} c++experimental)
if(ENABLE_LOGGING)
    target_link_libraries(utils logging)
endif(ENABLE_LOGGING)

add_library(logging "utils/logger.cc")
target_link_libraries(logging spdlog)
target_compile_definitions(logging PUBLIC XYCO_ENABLE_LOG)
