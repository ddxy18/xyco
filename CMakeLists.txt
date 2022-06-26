cmake_minimum_required(VERSION 3.16.0)

project(
  xyco
  VERSION 0.1.0
  DESCRIPTION "C++20 based coroutine library"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_CXX_FLAGS "-std=c++20 -stdlib=libc++ -pthread")
include(CPack)

enable_testing()

include(FetchContent)
FetchContent_Declare(googletest
                     GIT_REPOSITORY "https://github.com/google/googletest.git"
                     GIT_TAG main)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(GSL
                     GIT_REPOSITORY "https://github.com/microsoft/GSL.git"
                     GIT_TAG main)
FetchContent_MakeAvailable(GSL)
FetchContent_Declare(spdlog
                     GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
                     GIT_TAG v1.x)
FetchContent_MakeAvailable(spdlog)

include_directories(src)
include_directories(/usr/include/libunwind)

file(GLOB_RECURSE xyco_src_lib "src/fs/*.cc" "src/io/*.cc" "src/net/*.cc" "src/runtime/*.cc" "src/sync/*.cc" "src/time/*.cc" "src/utils/*.cc")

add_subdirectory(src)

add_subdirectory(tests)

add_subdirectory(benchmark)

add_subdirectory(examples)