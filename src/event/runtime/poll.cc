#include "poll.h"

#include "future.h"
#include "runtime.h"

reactor::Poll::Poll(std::unique_ptr<Registry> registry)
    : registry_(std::move(registry)) {}

auto reactor::Poll::registry() -> Registry * { return registry_.get(); }

auto reactor::Poll::poll(Events *events, int timeout) -> IoResult<void> {
  auto res = registry_->select(events, timeout);
  for (auto &ev : *events) {
    if (ev != nullptr && ev->future_ != nullptr) {
      TRACE("process event: fd={}\n", ev->fd_);
      runtime::RuntimeCtx::get_ctx()->register_future(ev->future_);
    }
  }
  events->clear();
  return res;
}