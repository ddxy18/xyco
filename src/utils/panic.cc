#include "panic.h"

#include "fmt/core.h"

auto panic() -> void {
  fmt::print("panic!\n");
  throw std::runtime_error("panic");
}