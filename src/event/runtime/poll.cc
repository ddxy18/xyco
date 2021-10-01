#include "poll.h"

#include "future.h"
#include "runtime_base.h"

auto reactor::Poll::poll(Events *events, int timeout) -> IoResult<Void> {
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