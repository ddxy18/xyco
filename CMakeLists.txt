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
FetchContent_Declare(
  GSL
  URL "https://github.com/microsoft/GSL/archive/refs/tags/v4.0.0.tar.gz"
  URL_HASH
    SHA256=f0e32cb10654fea91ad56bde89170d78cfbf4363ee0b01d8f097de2ba49f6ce9)
FetchContent_MakeAvailable(GSL)
FetchContent_Declare(
  spdlog
  URL "https://github.com/gabime/spdlog/archive/refs/tags/v1.10.0.tar.gz"
  URL_HASH
    SHA256=697f91700237dbae2326b90469be32b876b2b44888302afbc7aceb68bcfe8224)
FetchContent_MakeAvailable(spdlog)

find_package(Boost REQUIRED)

add_executable(xyco_epoll_main src/main.cc)
target_link_libraries(xyco_epoll_main io_epoll net_epoll runtime)

add_executable(xyco_uring_main src/main.cc)
target_link_libraries(xyco_uring_main io_uring net_uring runtime)

# Reusable library

add_library(future src/runtime/future.cc)
add_library(xyco::future ALIAS future)
target_sources(future PUBLIC include/xyco/runtime/future.h)
target_include_directories(future PUBLIC include)
target_link_libraries(future PUBLIC GSL ${Boost_STACKTRACE_ADDR2LINE_LIBRARY})

add_library(panic src/utils/panic.cc)
add_library(xyco::panic ALIAS panic)
target_sources(panic PUBLIC include/xyco/utils/panic.h)
target_include_directories(panic PUBLIC include)
target_link_libraries(panic PRIVATE ${Boost_STACKTRACE_ADDR2LINE_LIBRARY})

add_library(result INTERFACE)
add_library(xyco::result ALIAS result)
target_sources(result INTERFACE include/xyco/utils/result.h)
target_include_directories(result INTERFACE include)
target_link_libraries(result INTERFACE panic)

add_library(overload INTERFACE)
add_library(xyco::overload ALIAS overload)
target_sources(overload INTERFACE include/xyco/utils/overload.h)
target_include_directories(overload INTERFACE include)

add_library(logging src/utils/logger.cc)
add_library(xyco::logging ALIAS logging)
target_sources(logging PUBLIC include/xyco/utils/logger.h)
target_include_directories(logging PUBLIC include)
target_link_libraries(logging PUBLIC spdlog)
target_compile_definitions(logging PUBLIC XYCO_ENABLE_LOG)

# Runtime Core

add_library(error src/utils/error.cc)
add_library(xyco::error ALIAS error)
target_sources(error PUBLIC include/xyco/utils/error.h)
target_include_directories(error PUBLIC include)
target_link_libraries(error PUBLIC result)

add_library(runtime_ctx src/runtime/runtime_ctx.cc src/runtime/driver.cc)
add_library(xyco::runtime_ctx ALIAS runtime_ctx)
target_sources(
  runtime_ctx
  PUBLIC include/xyco/runtime/runtime_ctx.h include/xyco/runtime/driver.h
         include/xyco/runtime/registry.h
  PRIVATE include/xyco/runtime/runtime.h)
target_include_directories(runtime_ctx PUBLIC include)
target_link_libraries(
  runtime_ctx
  INTERFACE result error overload
  PUBLIC future)
if(XYCO_ENABLE_LOGGING)
  target_link_libraries(runtime_ctx INTERFACE logging)
endif(XYCO_ENABLE_LOGGING)

add_library(runtime src/runtime/runtime.cc)
add_library(xyco::runtime ALIAS runtime)
target_sources(
  runtime
  PUBLIC include/xyco/runtime/runtime.h
  PRIVATE include/xyco/runtime/driver.h include/xyco/runtime/registry.h)
target_include_directories(runtime PUBLIC include)
target_link_libraries(runtime PUBLIC runtime_ctx)

# Registry

add_library(task src/task/registry.cc)
add_library(xyco::task ALIAS task)
target_sources(
  task
  PUBLIC include/xyco/task/registry.h
  INTERFACE include/xyco/task/blocking_task.h include/xyco/task/join.h
            include/xyco/task/select.h)
target_include_directories(task PUBLIC include)
target_link_libraries(task PRIVATE runtime_ctx)

add_library(io_epoll src/io/epoll/registry.cc src/io/epoll/extra.cc)
add_library(xyco::io_epoll ALIAS io_epoll)
target_sources(
  io_epoll
  PUBLIC include/xyco/io/epoll/registry.h include/xyco/io/epoll/extra.h
  INTERFACE include/xyco/io/read.h include/xyco/io/buffer_reader.h
            include/xyco/io/seek.h include/xyco/io/write.h)
target_include_directories(
  io_epoll
  INTERFACE include/xyco/io/standalone/epoll
  PUBLIC include)
target_link_libraries(io_epoll PUBLIC runtime_ctx)
add_library(io_uring src/io/io_uring/registry.cc src/io/io_uring/extra.cc)
add_library(xyco::io_uring ALIAS io_uring)
target_sources(
  io_uring
  PUBLIC include/xyco/io/io_uring/registry.h include/xyco/io/io_uring/extra.h
  INTERFACE include/xyco/io/read.h include/xyco/io/buffer_reader.h
            include/xyco/io/seek.h include/xyco/io/write.h)
target_include_directories(
  io_uring
  INTERFACE include/xyco/io/standalone/io_uring
  PUBLIC include)
target_link_libraries(
  io_uring
  PRIVATE uring
  PUBLIC runtime_ctx)
add_library(io_all src/io/epoll/registry.cc src/io/epoll/extra.cc
                   src/io/io_uring/registry.cc src/io/io_uring/extra.cc)
add_library(xyco::io_all ALIAS io_all)
target_sources(
  io_all
  PUBLIC include/xyco/io/epoll/registry.h include/xyco/io/epoll/extra.h
         include/xyco/io/io_uring/registry.h include/xyco/io/io_uring/extra.h
  INTERFACE include/xyco/io/read.h include/xyco/io/buffer_reader.h
            include/xyco/io/seek.h include/xyco/io/write.h)
target_include_directories(io_all PUBLIC include)
target_link_libraries(
  io_all
  PRIVATE uring
  PUBLIC runtime_ctx)

add_library(net_epoll src/net/socket.cc src/net/epoll/listener.cc)
add_library(xyco::net_epoll ALIAS net_epoll)
target_sources(net_epoll PUBLIC include/xyco/net/socket.h
                                include/xyco/net/epoll/listener.h)
target_include_directories(
  net_epoll
  INTERFACE include/xyco/net/standalone/epoll
  PUBLIC include)
target_link_libraries(net_epoll PRIVATE io_epoll task runtime_ctx)
add_library(net_uring src/net/socket.cc src/net/io_uring/listener.cc)
add_library(xyco::net_uring ALIAS net_uring)
target_sources(net_uring PUBLIC include/xyco/net/socket.h
                                include/xyco/net/io_uring/listener.h)
target_include_directories(
  net_uring
  INTERFACE include/xyco/net/standalone/io_uring
  PUBLIC include)
target_link_libraries(net_uring PRIVATE io_uring task runtime_ctx)
add_library(net_all src/net/socket.cc src/net/epoll/listener.cc
                    src/net/io_uring/listener.cc)
add_library(xyco::net_all ALIAS net_all)
target_sources(
  net_all PUBLIC include/xyco/net/socket.h include/xyco/net/epoll/listener.h
                 include/xyco/net/io_uring/listener.h)
target_include_directories(net_all PUBLIC include)
target_link_libraries(net_all PRIVATE io_all task runtime_ctx)

add_library(fs_epoll src/fs/utils.cc src/fs/epoll/file.cc)
add_library(xyco::fs_epoll ALIAS fs_epoll)
target_sources(fs_epoll PUBLIC include/xyco/fs/utils.h
                               include/xyco/fs/epoll/file.h)
target_include_directories(
  fs_epoll
  INTERFACE include/xyco/fs/standalone/epoll
  PUBLIC include)
target_link_libraries(fs_epoll PRIVATE io_epoll task runtime_ctx)
add_library(fs_uring src/fs/utils.cc src/fs/io_uring/file.cc)
add_library(xyco::fs_uring ALIAS fs_uring)
target_sources(fs_uring PUBLIC include/xyco/fs/utils.h
                               include/xyco/fs/io_uring/file.h)
target_include_directories(
  fs_uring
  INTERFACE include/xyco/fs/standalone/io_uring
  PUBLIC include)
target_link_libraries(fs_uring PRIVATE io_uring task runtime_ctx)
add_library(fs_all src/fs/utils.cc src/fs/epoll/file.cc src/fs/io_uring/file.cc)
add_library(xyco::fs_all ALIAS fs_all)
target_sources(
  fs_all PUBLIC include/xyco/fs/utils.h include/xyco/fs/epoll/file.h
                include/xyco/fs/io_uring/file.h)
target_include_directories(fs_all PUBLIC include)
target_link_libraries(fs_all PRIVATE io_all task runtime_ctx)

add_library(time src/time/driver.cc src/time/wheel.cc src/time/clock.cc)
add_library(xyco::time ALIAS time)
target_sources(
  time
  PUBLIC include/xyco/time/driver.h include/xyco/time/clock.h
  INTERFACE include/xyco/time/timeout.h include/xyco/time/sleep.h
  PRIVATE include/xyco/time/wheel.h)
target_include_directories(time PUBLIC include)
target_link_libraries(time PRIVATE runtime_ctx)

option(XYCO_ENABLE_LOGGING "OFF")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  FetchContent_Declare(
    googletest
    URL "https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz"
    URL_HASH
      SHA256=81964fe578e9bd7c94dfdb09c8e4d6e6759e19967e397dbea48d1c10e45d0df2)
  FetchContent_MakeAvailable(googletest)
  FetchContent_Declare(
    asio
    URL "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-28-0.tar.gz"
    URL_HASH
      SHA256=226438b0798099ad2a202563a83571ce06dd13b570d8fded4840dbc1f97fa328)
  FetchContent_MakeAvailable(asio)

  add_subdirectory(tests)
  add_subdirectory(benchmark)
  add_subdirectory(examples)
endif()
