#include "driver.h"

#include "runtime/future.h"
#include "runtime/runtime.h"

[[nodiscard]] auto io::IoRegistry::Register(reactor::Event& ev,
                                            reactor::Interest interest)
    -> IoResult<void> {
  return register_local(ev, interest);
}

[[nodiscard]] auto io::IoRegistry::reregister(reactor::Event& ev,
                                              reactor::Interest interest)
    -> IoResult<void> {
  return reregister_local(ev, interest);
}

[[nodiscard]] auto io::IoRegistry::deregister(reactor::Event& ev,
                                              reactor::Interest interest)
    -> IoResult<void> {
  return deregister_local(ev, interest);
}

[[nodiscard]] auto io::IoRegistry::register_local(reactor::Event& ev,
                                                  reactor::Interest interest)
    -> IoResult<void> {
  auto current_thread =
      runtime::RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().Register(ev, interest);
}

[[nodiscard]] auto io::IoRegistry::reregister_local(reactor::Event& ev,
                                                    reactor::Interest interest)
    -> IoResult<void> {
  auto current_thread =
      runtime::RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().reregister(ev, interest);
}

[[nodiscard]] auto io::IoRegistry::deregister_local(reactor::Event& ev,
                                                    reactor::Interest interest)
    -> IoResult<void> {
  auto current_thread =
      runtime::RuntimeCtx::get_ctx()->workers_.find(std::this_thread::get_id());
  return current_thread->second->get_epoll_registry().deregister(ev, interest);
}

[[nodiscard]] auto io::IoRegistry::select(reactor::Events& events, int timeout)
    -> IoResult<void> {
  return IoResult<void>::ok();
}