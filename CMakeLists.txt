cmake_minimum_required(VERSION 3.16.0)

project(
  xyco
  VERSION 0.1.0
  DESCRIPTION "C++20 based coroutine library"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-std=c++20 -stdlib=libc++ -fexperimental-library)
add_link_options(-lc++ -lm -rdynamic)

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
                     URL "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-24-0.tar.gz"
                     URL_HASH SHA256=cbcaaba0f66722787b1a7c33afe1befb3a012b5af3ad7da7ff0f6b8c9b7a8a5b)
FetchContent_MakeAvailable(asio)

include_directories(src)

find_package(Boost REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmark)
add_subdirectory(examples)
