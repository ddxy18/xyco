#include "poll.h"

#include <vector>

#include "future.h"
#include "runtime_base.h"

auto reactor::Poll::poll(Events *events, int timeout) -> IoResult<Void> {
  auto res = registry_->select(events, timeout);
  for (auto &ev : *events) {
    if (ev != nullptr && ev->future_ != nullptr) {
      fmt::print("porcess event:{{fd={}}}\n", ev->fd_);
      runtime::RuntimeCtx::get_ctx()->register_future(ev->future_);
    }
    // ev->future_ = nullptr;
    // ev = nullptr;
  }
  events->clear();
  return res;
}