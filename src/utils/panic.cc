#include "panic.h"

#include <format>
#include <iostream>

#include "boost/stacktrace.hpp"

auto xyco::utils::panic(const std::string &info) -> void {
  auto unwind_info = std::format("Panic:{}\n{}", info,
                                 to_string(boost::stacktrace::stacktrace()));

  std::cerr << unwind_info;
  throw std::runtime_error(unwind_info);
}
