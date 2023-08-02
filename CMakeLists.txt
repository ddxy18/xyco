cmake_minimum_required(VERSION 3.24.0)

project(
  xyco
  VERSION 0.1.0
  DESCRIPTION "C++20 based coroutine library"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-std=c++20 -stdlib=libc++ -fexperimental-library)
add_link_options(-fuse-ld=lld-17 -lc++ -lm -rdynamic)
cmake_policy(SET CMP0135 NEW)

include(FetchContent)
FetchContent_Declare(googletest
                     URL "https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz"
                     URL_HASH SHA256=81964fe578e9bd7c94dfdb09c8e4d6e6759e19967e397dbea48d1c10e45d0df2)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(GSL
                     URL "https://github.com/microsoft/GSL/archive/refs/tags/v4.0.0.tar.gz"
                     URL_HASH SHA256=f0e32cb10654fea91ad56bde89170d78cfbf4363ee0b01d8f097de2ba49f6ce9)
FetchContent_MakeAvailable(GSL)
FetchContent_Declare(spdlog
                     URL "https://github.com/gabime/spdlog/archive/refs/tags/v1.10.0.tar.gz"
                     URL_HASH SHA256=697f91700237dbae2326b90469be32b876b2b44888302afbc7aceb68bcfe8224)
FetchContent_MakeAvailable(spdlog)
FetchContent_Declare(asio
                     URL "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-28-0.tar.gz"
                     URL_HASH SHA256=226438b0798099ad2a202563a83571ce06dd13b570d8fded4840dbc1f97fa328)
FetchContent_MakeAvailable(asio)

include_directories(include)

find_package(Boost REQUIRED)

add_subdirectory(tests)
add_subdirectory(benchmark)
add_subdirectory(examples)

add_executable(xyco_epoll_main src/main.cc)
target_link_libraries(xyco_epoll_main io_epoll net_epoll runtime)

add_executable(xyco_uring_main src/main.cc)
target_link_libraries(xyco_uring_main io_uring net_uring runtime)

set(fs_epoll_src "src/fs/utils.cc" "src/fs/epoll/file.cc")
set(fs_uring_src "src/fs/utils.cc" "src/fs/io_uring/file.cc")
set(fs_all_src "src/fs/utils.cc" "src/fs/epoll/file.cc" "src/fs/io_uring/file.cc")

set(io_epoll_src "src/io/epoll/registry.cc" "src/io/epoll/extra.cc")
set(io_uring_src "src/io/io_uring/registry.cc" "src/io/io_uring/extra.cc")
set(io_all_src "src/io/epoll/registry.cc" "src/io/epoll/extra.cc" "src/io/io_uring/registry.cc" "src/io/io_uring/extra.cc")

set(net_epoll_src "src/net/socket.cc" "src/net/epoll/listener.cc")
set(net_uring_src "src/net/socket.cc" "src/net/io_uring/listener.cc")
set(net_all_src "src/net/socket.cc" "src/net/epoll/listener.cc" "src/net/io_uring/listener.cc")

set(task_src "src/task/registry.cc")

set(runtime_src "src/runtime/driver.cc" "src/runtime/future.cc" "src/runtime/runtime.cc" "src/runtime/runtime_ctx.cc")

set(time_src "src/time/driver.cc" "src/time/wheel.cc" "src/time/clock.cc")

set(utils_src "src/utils/error.cc" "src/utils/panic.cc")

add_library(fs_epoll ${fs_epoll_src})
target_link_libraries(fs_epoll io_epoll task runtime utils)
target_include_directories(fs_epoll INTERFACE include/xyco/fs/standalone/epoll)
add_library(fs_uring ${fs_uring_src})
target_link_libraries(fs_uring io_uring task runtime utils uring)
target_include_directories(fs_uring INTERFACE include/xyco/fs/standalone/io_uring)
add_library(fs_all ${fs_all_src})
target_link_libraries(fs_all io_all task runtime utils uring)

add_library(io_epoll ${io_epoll_src})
target_link_libraries(io_epoll runtime utils spdlog)
target_include_directories(io_epoll INTERFACE include/xyco/io/standalone/epoll)
add_library(io_uring ${io_uring_src})
target_link_libraries(io_uring runtime utils spdlog uring)
target_include_directories(io_uring INTERFACE include/xyco/io/standalone/io_uring)
add_library(io_all ${io_all_src})
target_link_libraries(io_all runtime utils spdlog uring)

add_library(net_epoll ${net_epoll_src})
target_link_libraries(net_epoll io_epoll task runtime utils spdlog)
target_include_directories(net_epoll INTERFACE include/xyco/net/standalone/epoll)
add_library(net_uring ${net_uring_src})
target_link_libraries(net_uring io_uring task runtime utils spdlog uring)
target_include_directories(net_uring INTERFACE include/xyco/net/standalone/io_uring)
add_library(net_all ${net_all_src})
target_link_libraries(net_all io_all task runtime utils spdlog uring)

add_library(task ${task_src})
target_link_libraries(task GSL runtime)

add_library(runtime ${runtime_src})
target_link_libraries(runtime utils GSL)

add_library(time ${time_src})
target_link_libraries(time runtime spdlog)

option(XYCO_ENABLE_LOGGING "OFF")

add_library(utils ${utils_src})
target_link_libraries(utils spdlog ${Boost_STACKTRACE_ADDR2LINE_LIBRARY})
if(XYCO_ENABLE_LOGGING)
    target_link_libraries(utils logging)
endif(XYCO_ENABLE_LOGGING)

add_library(logging "src/utils/logger.cc")
target_link_libraries(logging spdlog)
target_compile_definitions(logging PUBLIC XYCO_ENABLE_LOG)
