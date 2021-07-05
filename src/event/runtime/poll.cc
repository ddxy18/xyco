#include "poll.h"

#include "runtime.h"

auto reactor::Poll::poll(Events *events, int timeout) -> IoResult<Void> {
  auto res = registry_->select(events, timeout);
  for (auto &ev : *events) {
    if (ev == nullptr) {
      break;
    }
    if (ev->future_ != nullptr) {
      fmt::print("porcess event:{{fd={}}}\n", ev->fd_);
      runtime::runtime->register_future(ev->future_);
    }
    ev->future_ = nullptr;
  }
  return res;
}