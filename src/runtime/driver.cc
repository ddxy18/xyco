#include "driver.h"

#include "runtime.h"

auto xyco::runtime::Driver::poll() -> void {
  runtime::Events events;
  io_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();
  RuntimeCtx::get_ctx()->wake(events);

  time_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();
  RuntimeCtx::get_ctx()->wake(events);

  blocking_registry_.select(events, net::NetRegistry::MAX_TIMEOUT_MS).unwrap();
  RuntimeCtx::get_ctx()->wake(events);
}

auto xyco::runtime::Driver::net_handle() -> io::IoRegistry* {
  return &io_registry_;
}

auto xyco::runtime::Driver::time_handle() -> time::TimeRegistry* {
  return &time_registry_;
}

auto xyco::runtime::Driver::blocking_handle() -> BlockingRegistry* {
  return &blocking_registry_;
}

xyco::runtime::Driver::Driver(uintptr_t blocking_num)
    : blocking_registry_(blocking_num) {}