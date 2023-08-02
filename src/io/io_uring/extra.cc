#include "xyco/io/io_uring/extra.h"

auto xyco::io::uring::IoExtra::print() const -> std::string {
  return std::format("{}", *this);
}
