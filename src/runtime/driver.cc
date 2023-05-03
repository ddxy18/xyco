#include "driver.h"

#include "runtime.h"

auto xyco::runtime::Driver::poll() -> void {
  runtime::Events events;

  auto& local_registry =
      local_registries_.find(std::this_thread::get_id())->second;
  for (auto& [key, registry] : local_registry) {
    registry->select(events, MAX_TIMEOUT).unwrap();
    RuntimeCtx::get_ctx()->wake(events);
  }
}

auto xyco::runtime::Driver::add_thread() -> void {
  local_registries_[std::this_thread::get_id()] = std::remove_reference_t<
      decltype(local_registries_[std::this_thread::get_id()])>();
  for (auto& registry_init : registry_initializers_) {
    registry_init();
  }
}
