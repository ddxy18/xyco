#include "driver.h"

#include "runtime/future.h"
#include "runtime/runtime.h"

auto xyco::io::IoRegistry::Register(runtime::Event& ev) -> IoResult<void> {
  return register_local(ev);
}

auto xyco::io::IoRegistry::reregister(runtime::Event& ev) -> IoResult<void> {
  return reregister_local(ev);
}

auto xyco::io::IoRegistry::deregister(runtime::Event& ev) -> IoResult<void> {
  return deregister_local(ev);
}

auto xyco::io::IoRegistry::register_local(runtime::Event& ev)
    -> IoResult<void> {
  auto current_thread =
      runtime::RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().Register(ev);
}

auto xyco::io::IoRegistry::reregister_local(runtime::Event& ev)
    -> IoResult<void> {
  auto current_thread =
      runtime::RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().reregister(ev);
}

auto xyco::io::IoRegistry::deregister_local(runtime::Event& ev)
    -> IoResult<void> {
  auto current_thread =
      runtime::RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().deregister(ev);
}

auto xyco::io::IoRegistry::select(runtime::Events& events,
                                  std::chrono::milliseconds timeout)
    -> IoResult<void> {
  return IoResult<void>::ok();
}