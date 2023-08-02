#include "xyco/io/epoll/extra.h"

auto xyco::io::epoll::IoExtra::print() const -> std::string {
  return std::format("{}", *this);
}

xyco::io::epoll::IoExtra::IoExtra(Interest interest, int file_descriptor)
    : state_(), interest_(interest), fd_(file_descriptor) {}
