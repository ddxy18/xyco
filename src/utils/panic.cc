#include "panic.h"

#include <iostream>

#include "boost/stacktrace.hpp"
#include "logger.h"
#include "spdlog/formatter.h"

auto xyco::utils::panic(std::string info) -> void {
  auto unwind_info = fmt::format("Panic:{}\n{}", info,
                                 to_string(boost::stacktrace::stacktrace()));

  std::cerr << unwind_info;
  ERROR(unwind_info);
  throw std::runtime_error(unwind_info);
}
