#include "panic.h"

#include "logger.h"

auto panic() -> void {
  TRACE("panic!");
  throw std::runtime_error("panic");
}