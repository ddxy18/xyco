cmake_minimum_required(VERSION 3.16.0)
project(
  xywebserver
  VERSION 0.1.0
  DESCRIPTION "A http web server demo"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

enable_testing()

include(FetchContent)
FetchContent_Declare(googletest
                     GIT_REPOSITORY https://github.com/google/googletest.git)
FetchContent_MakeAvailable(googletest)

add_subdirectory(src)

add_subdirectory(tests)
