#include "extra.h"

auto xyco::io::uring::IoExtra::print() const -> std::string {
  return std::format("{}", *this);
}
