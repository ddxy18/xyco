cmake_minimum_required(VERSION 3.28.0)

project(
  xyco
  VERSION 0.1.0
  DESCRIPTION "C++20 based coroutine library"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

# Add these options to avoid clang-diagnostic-module-file-config-mismatch error
add_compile_options(-fstack-protector-all -fwrapv)
add_link_options(-fuse-ld=lld -rdynamic -stdlib=libc++)
cmake_policy(SET CMP0135 NEW)

include(FetchContent)
FetchContent_Declare(
  GSL
  URL "https://github.com/microsoft/GSL/archive/refs/tags/v4.0.0.tar.gz"
  URL_HASH
    SHA256=f0e32cb10654fea91ad56bde89170d78cfbf4363ee0b01d8f097de2ba49f6ce9)
FetchContent_MakeAvailable(GSL)

set(SPDLOG_USE_STD_FORMAT ON)
FetchContent_Declare(
  spdlog
  URL "https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz"
  URL_HASH
    SHA256=4dccf2d10f410c1e2feaff89966bfc49a1abb29ef6f08246335b110e001e09a9)
FetchContent_MakeAvailable(spdlog)

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
endif()

# External dependencies ought to be declared ahead to skip linting for them
if(XYCO_ENABLE_LINTING)
  add_compile_options(-Wall -Wextra -Wdeprecated -Werror)
  set(CMAKE_CXX_CLANG_TIDY
      clang-tidy
      # All BMI locations should be added manually here as the
      # explanation(https://discourse.llvm.org/t/clang-tidy-doesnt-find-standard-c-modules-despite-providing-compilation-database-and-prebuilt-modules-path/71326/2)
      # instructs in the solution (2).
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_error.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_future.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_overload.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_panic.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_logging.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_runtime_core.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_runtime_ctx.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_runtime.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_task.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_time.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_sync.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_io_all.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_io_epoll.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_io_uring.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_io_common.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_net_all.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_net_epoll.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_net_uring.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_net_common.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_fs_all.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_fs_epoll.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_fs_uring.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_fs_common.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/CMakeFiles/xyco_libc.dir/
      --extra-arg=-fprebuilt-module-path=${CMAKE_BINARY_DIR}/tests/CMakeFiles/xyco_test_common_utils.dir/
  )
endif(XYCO_ENABLE_LINTING)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  add_subdirectory(tests)
  add_subdirectory(benchmark)
  add_subdirectory(examples)
endif()

find_package(Boost REQUIRED)

add_executable(xyco_epoll_main src/main.cc)
target_link_libraries(xyco_epoll_main xyco::io_epoll xyco::net_epoll
                      xyco::runtime)

add_executable(xyco_uring_main src/main.cc)
target_link_libraries(xyco_uring_main xyco::io_uring xyco::net_uring
                      xyco::runtime)

# Reusable library

add_library(xyco_libc)
add_library(xyco::libc ALIAS xyco_libc)
target_sources(xyco_libc PUBLIC FILE_SET libc_module TYPE CXX_MODULES FILES
                                include/xyco/utils/libc.ccm)

add_library(xyco_future src/runtime/future.cc)
add_library(xyco::future ALIAS xyco_future)
target_sources(xyco_future PUBLIC FILE_SET future_module TYPE CXX_MODULES FILES
                                  include/xyco/runtime/future.ccm)
target_link_libraries(xyco_future PUBLIC GSL
                                         ${Boost_STACKTRACE_ADDR2LINE_LIBRARY})

add_library(xyco_panic src/utils/panic.cc)
add_library(xyco::panic ALIAS xyco_panic)
target_sources(xyco_panic PUBLIC FILE_SET panic_module TYPE CXX_MODULES FILES
                                 include/xyco/utils/panic.ccm)
target_link_libraries(xyco_panic PRIVATE ${Boost_STACKTRACE_ADDR2LINE_LIBRARY})

add_library(xyco_result INTERFACE)
add_library(xyco::result ALIAS xyco_result)
target_sources(xyco_result INTERFACE include/xyco/utils/result.h)
target_include_directories(xyco_result INTERFACE include)

add_library(xyco_overload)
add_library(xyco::overload ALIAS xyco_overload)
target_sources(xyco_overload PUBLIC FILE_SET overload_module TYPE CXX_MODULES
                                    FILES include/xyco/utils/overload.ccm)

add_library(xyco_logging)
target_link_libraries(xyco_logging PUBLIC spdlog)
if(XYCO_ENABLE_LOGGING)
  target_sources(
    xyco_logging
    PUBLIC FILE_SET logging_module TYPE CXX_MODULES FILES
           include/xyco/utils/logging.ccm
    PRIVATE src/utils/logging.cc)
  target_compile_definitions(xyco_logging PUBLIC XYCO_ENABLE_LOGGING)
else()
  target_sources(xyco_logging PUBLIC FILE_SET logging_module TYPE CXX_MODULES
                                     FILES include/xyco/utils/logging.ccm)
endif(XYCO_ENABLE_LOGGING)
add_library(xyco::logging ALIAS xyco_logging)

# Runtime Core

add_library(xyco_error src/utils/error.cc)
add_library(xyco::error ALIAS xyco_error)
target_sources(xyco_error PUBLIC FILE_SET error_module TYPE CXX_MODULES FILES
                                 include/xyco/utils/error.ccm)

add_library(xyco_runtime_core src/runtime/driver.cc src/runtime/core.cc)
target_sources(
  xyco_runtime_core
  PUBLIC FILE_SET
         runtime_core_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/runtime/driver.ccm
         include/xyco/runtime/registry.ccm
         include/xyco/runtime/core.ccm)
target_link_libraries(xyco_runtime_core PUBLIC xyco::error xyco::future
                                               xyco::logging)

add_library(xyco_runtime_ctx src/runtime/runtime_ctx.cc)
add_library(xyco::runtime_ctx ALIAS xyco_runtime_ctx)
target_sources(
  xyco_runtime_ctx
  PUBLIC FILE_SET
         runtime_ctx_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/runtime/runtime_ctx.ccm
         include/xyco/runtime/global_registry.ccm
         include/xyco/runtime/thread_local_registry.ccm)
target_link_libraries(
  xyco_runtime_ctx
  INTERFACE xyco::result xyco::overload xyco::panic
  PUBLIC xyco_runtime_core)

add_library(xyco_runtime src/runtime/runtime.cc)
add_library(xyco::runtime ALIAS xyco_runtime)
target_sources(xyco_runtime PUBLIC FILE_SET runtime_module TYPE CXX_MODULES
                                   FILES include/xyco/runtime/runtime.ccm)
target_link_libraries(xyco_runtime PUBLIC xyco_runtime_core)

# Registry

add_library(xyco_task src/task/registry.cc)
add_library(xyco::task ALIAS xyco_task)
target_sources(
  xyco_task
  PUBLIC FILE_SET
         task_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/task/registry.ccm
         include/xyco/task/blocking_task.ccm
         include/xyco/task/join.ccm
         include/xyco/task/select.ccm
         include/xyco/task/mod.ccm)
target_link_libraries(
  xyco_task
  PUBLIC xyco::future
  PRIVATE xyco::runtime_ctx)

add_library(xyco_io_common)
target_sources(
  xyco_io_common
  PUBLIC FILE_SET
         io_common_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/io/common.ccm
         include/xyco/io/read.ccm
         include/xyco/io/buffer_reader.ccm
         include/xyco/io/seek.ccm
         include/xyco/io/utils.ccm
         include/xyco/io/write.ccm)
target_link_libraries(
  xyco_io_common
  PUBLIC xyco::future
  PRIVATE xyco::runtime_ctx xyco::libc)
add_library(xyco_io_epoll src/io/epoll/registry.cc)
add_library(xyco::io_epoll ALIAS xyco_io_epoll)
target_sources(
  xyco_io_epoll
  PUBLIC FILE_SET
         io_epoll_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/io/epoll/mod.ccm
         include/xyco/io/epoll/registry.ccm)
target_link_libraries(
  xyco_io_epoll
  PUBLIC xyco_io_common
  PRIVATE xyco::runtime_ctx xyco::libc)
add_library(xyco_io_uring src/io/io_uring/registry.cc)
add_library(xyco::io_uring ALIAS xyco_io_uring)
target_sources(
  xyco_io_uring
  PUBLIC FILE_SET
         io_uring_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/io/io_uring/mod.ccm
         include/xyco/io/io_uring/registry.ccm)
target_link_libraries(
  xyco_io_uring
  PUBLIC xyco_io_common
  PRIVATE xyco::runtime_ctx uring xyco::libc)
add_library(xyco_io_all src/io/epoll/registry.cc src/io/io_uring/registry.cc)
add_library(xyco::io_all ALIAS xyco_io_all)
target_sources(
  xyco_io_all
  PUBLIC FILE_SET
         io_all_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/io/mod.ccm
         include/xyco/io/epoll/registry.ccm
         include/xyco/io/io_uring/registry.ccm)
target_link_libraries(
  xyco_io_all
  PUBLIC xyco_io_common
  PRIVATE xyco::runtime_ctx uring xyco::libc)

add_library(xyco_net_common src/net/socket.cc)
target_sources(
  xyco_net_common
  PUBLIC FILE_SET
         net_common_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/net/common.ccm
         include/xyco/net/socket.ccm)
target_link_libraries(
  xyco_net_common
  INTERFACE xyco::future
  PRIVATE xyco::runtime_ctx xyco::libc)
add_library(xyco_net_epoll src/net/epoll/listener.cc)
add_library(xyco::net_epoll ALIAS xyco_net_epoll)
target_sources(
  xyco_net_epoll
  PUBLIC FILE_SET
         net_epoll_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/net/epoll/mod.ccm
         include/xyco/net/epoll/listener.ccm)
target_link_libraries(
  xyco_net_epoll
  PUBLIC xyco_net_common
  PRIVATE xyco::io_epoll xyco::task xyco::runtime_ctx xyco::libc)
add_library(xyco_net_uring src/net/io_uring/listener.cc)
add_library(xyco::net_uring ALIAS xyco_net_uring)
target_sources(
  xyco_net_uring
  PUBLIC FILE_SET
         net_uring_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/net/io_uring/mod.ccm
         include/xyco/net/io_uring/listener.ccm)
target_link_libraries(
  xyco_net_uring
  PUBLIC xyco_net_common
  PRIVATE xyco::io_uring xyco::task xyco::runtime_ctx xyco::libc)
add_library(xyco_net_all src/net/epoll/listener.cc src/net/io_uring/listener.cc)
add_library(xyco::net_all ALIAS xyco_net_all)
target_sources(
  xyco_net_all
  PUBLIC FILE_SET
         net_all_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/net/mod.ccm
         include/xyco/net/epoll/listener.ccm
         include/xyco/net/io_uring/listener.ccm)
target_link_libraries(
  xyco_net_all
  PUBLIC xyco_net_common
  PRIVATE xyco::io_all xyco::task xyco::runtime_ctx xyco::libc)

add_library(xyco_fs_common src/fs/utils.cc)
target_sources(
  xyco_fs_common
  PUBLIC FILE_SET
         fs_common_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/fs/common.ccm
         include/xyco/fs/file_common.ccm
         include/xyco/fs/utils.ccm)
target_link_libraries(
  xyco_fs_common
  PUBLIC xyco::future
  PRIVATE xyco::task xyco::runtime_ctx xyco::libc)
add_library(xyco_fs_epoll src/fs/epoll/file.cc)
add_library(xyco::fs_epoll ALIAS xyco_fs_epoll)
target_sources(
  xyco_fs_epoll
  PUBLIC FILE_SET
         fs_epoll_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/fs/epoll/mod.ccm
         include/xyco/fs/epoll/file.ccm)
target_link_libraries(
  xyco_fs_epoll
  PUBLIC xyco_fs_common
  PRIVATE xyco::io_epoll xyco::task xyco::runtime_ctx xyco::libc)
add_library(xyco_fs_uring src/fs/io_uring/file.cc)
add_library(xyco::fs_uring ALIAS xyco_fs_uring)
target_sources(
  xyco_fs_uring
  PUBLIC FILE_SET
         fs_uring_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/fs/io_uring/mod.ccm
         include/xyco/fs/io_uring/file.ccm)
target_link_libraries(
  xyco_fs_uring
  PUBLIC xyco_fs_common
  PRIVATE xyco::io_uring xyco::task xyco::runtime_ctx xyco::libc)
add_library(xyco_fs_all src/fs/epoll/file.cc src/fs/io_uring/file.cc)
add_library(xyco::fs_all ALIAS xyco_fs_all)
target_sources(
  xyco_fs_all
  PUBLIC FILE_SET
         fs_all_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/fs/mod.ccm
         include/xyco/fs/epoll/file.ccm
         include/xyco/fs/io_uring/file.ccm)
target_link_libraries(
  xyco_fs_all
  PUBLIC xyco_fs_common
  PRIVATE xyco::io_all xyco::task xyco::runtime_ctx xyco::libc)

add_library(xyco_time src/time/registry.cc src/time/wheel.cc src/time/clock.cc)
add_library(xyco::time ALIAS xyco_time)
target_sources(
  xyco_time
  PUBLIC FILE_SET
         time_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/time/mod.ccm
         include/xyco/time/registry.ccm
         include/xyco/time/clock.ccm
         include/xyco/time/timeout.ccm
         include/xyco/time/sleep.ccm
         include/xyco/time/wheel.ccm)
target_link_libraries(
  xyco_time
  PUBLIC xyco::future
  PRIVATE xyco::runtime_ctx xyco::task)

add_library(xyco_sync)
add_library(xyco::sync ALIAS xyco_sync)
target_sources(
  xyco_sync
  PUBLIC FILE_SET
         sync_module
         TYPE
         CXX_MODULES
         FILES
         include/xyco/sync/mod.ccm
         include/xyco/sync/mpsc.ccm
         include/xyco/sync/oneshot.ccm)
target_link_libraries(
  xyco_sync
  PUBLIC xyco::future
  PRIVATE xyco::runtime_ctx)

option(XYCO_ENABLE_LOGGING "OFF")
