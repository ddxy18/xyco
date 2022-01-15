#include "driver.h"

#include "net/driver/epoll.h"
#include "runtime.h"

auto xyco::runtime::Driver::poll() -> void {
  runtime::Events events;

  io_registry_.select(events, net::NetRegistry::MAX_TIMEOUT).unwrap();
  RuntimeCtx::get_ctx()->wake(events);

  local_registries_.find(std::this_thread::get_id())
      ->second->select(events, net::NetRegistry::MAX_TIMEOUT)
      .unwrap();
  RuntimeCtx::get_ctx()->wake_local(events);

  time_registry_.select(events, net::NetRegistry::MAX_TIMEOUT).unwrap();
  RuntimeCtx::get_ctx()->wake(events);

  blocking_registry_.select(events, net::NetRegistry::MAX_TIMEOUT).unwrap();
  RuntimeCtx::get_ctx()->wake(events);
}

auto xyco::runtime::Driver::io_handle() -> GlobalRegistry* {
  return &io_registry_;
}

auto xyco::runtime::Driver::time_handle() -> Registry* {
  return &time_registry_;
}

auto xyco::runtime::Driver::blocking_handle() -> Registry* {
  return &blocking_registry_;
}

auto xyco::runtime::Driver::local_handle() -> Registry* {
  return local_registries_.find(std::this_thread::get_id())->second.get();
}

auto xyco::runtime::Driver::add_registry(std::unique_ptr<Registry> registry)
    -> void {
  local_registries_[std::this_thread::get_id()] = std::move(registry);
}

xyco::runtime::Driver::Driver(uintptr_t blocking_num)
    : blocking_registry_(blocking_num) {}