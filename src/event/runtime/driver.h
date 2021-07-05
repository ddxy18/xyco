#ifndef XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_
#define XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "event/io/utils.h"
#include "event/net/epoll.h"
#include "event/runtime/blocking.h"
#include "event/runtime/poll.h"
#include "utils/result.h"

namespace runtime {
class BlockingPoll : public reactor::Registry {
 public:
  explicit BlockingPoll(int woker_num) : pool_(woker_num) { pool_.run(); }

  [[nodiscard]] auto Register(reactor::Event* ev) -> IoResult<Void> override {
    events_.push_back(ev);
    pool_.spawn(blocking::Task(
        *static_cast<std::function<void()>*>(ev->before_extra_)));
    return Ok<Void, IoError>(Void());
  }

  [[nodiscard]] auto reregister(reactor::Event* ev) -> IoResult<Void> override {
    return Ok<Void, IoError>(Void());
  }

  [[nodiscard]] auto deregister(reactor::Event* ev) -> IoResult<Void> override {
    return Ok<Void, IoError>(Void());
  }

  [[nodiscard]] auto select(reactor::Events* events, int timeout)
      -> IoResult<Void> override {
    auto i = 0;
    for (auto& ev : events_) {
      if (ev->after_extra_ != nullptr) {
        events->at(i++) = ev;
      }
    }
    return Ok<Void, IoError>(Void());
  }

 private:
  reactor::Events events_;
  blocking::BlockingPool pool_;
};

class Driver {
 public:
  explicit Driver(uintptr_t blocking_num)
      : net_poll_(std::make_unique<net::Epoll>()),
        blocking_poll_(std::make_unique<BlockingPoll>(blocking_num)),
        events_(net::Epoll::MAX_EVENTS) {}

  auto poll() -> void;

  auto net_handle() -> reactor::Poll* { return &net_poll_; }

  auto blocking_handle() -> reactor::Poll* { return &blocking_poll_; }

 private:
  reactor::Events events_;
  reactor::Poll net_poll_;
  reactor::Poll blocking_poll_;
};
}  // namespace runtime

#endif  // XYWEBSERVER_EVENT_RUNTIME_DRIVER_H_