cmake_minimum_required(VERSION 3.16.0)

project(
  xywebserver
  VERSION 0.1.0
  DESCRIPTION "A http web server demo"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CMAKE_CXX_FLAGS "-std=c++20 -stdlib=libc++ -pthread")
include(CPack)

enable_testing()

include(FetchContent)
FetchContent_Declare(googletest
                     GIT_REPOSITORY "https://gitee.com/mirrors/googletest.git")
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(GSL GIT_REPOSITORY "https://gitee.com/mirrors/GSL")
FetchContent_MakeAvailable(GSL)
FetchContent_Declare(spdlog
                     GIT_REPOSITORY "https://gitee.com/ywh_welcom/spdlog.git"
                     GIT_TAG v1.x)
FetchContent_MakeAvailable(spdlog)

include_directories(src tests)

add_subdirectory(src)

add_subdirectory(tests)
