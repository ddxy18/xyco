#include "panic.h"

#include "logger.h"

auto panic() -> void {
  TRACE("panic!\n");
  throw std::runtime_error("panic");
}