#include "driver.h"

#include "io/utils.h"
#include "net/driver/epoll.h"
#include "runtime/future.h"
#include "runtime/runtime.h"

auto xyco::io::IoExtra::print() const -> std::string {
  std::string state = "[";
  if ((state_.field_ & xyco::io::IoExtra::State::Registered) != 0) {
    state += "Registered,";
  }
  if ((state_.field_ & xyco::io::IoExtra::State::Pending) != 0) {
    state += "Pending,";
  }
  if ((state_.field_ & xyco::io::IoExtra::State::Readable) != 0) {
    state += "Readable,";
  }
  if ((state_.field_ & xyco::io::IoExtra::State::Writable) != 0) {
    state += "Writable,";
  }
  if ((state_.field_ & xyco::io::IoExtra::State::Error) != 0) {
    state += "Error,";
  }
  state[state.length() - 1] = ']';

  std::string interest;
  switch (interest_) {
    case xyco::io::IoExtra::Interest::Read:
      interest = "Read";
      break;
    case xyco::io::IoExtra::Interest::Write:
      interest = "Write";
      break;
    case xyco::io::IoExtra::Interest::All:
      interest = "All";
      break;
  }

  return fmt::format("IoExtra{{state_={}, interest_={}, fd_={}}}", state,
                     interest, fd_);
}

xyco::io::IoExtra::IoExtra(Interest interest, int file_descriptor)
    : state_(), interest_(interest), fd_(file_descriptor) {}

auto xyco::io::IoRegistry::Register(std::shared_ptr<runtime::Event> event)
    -> IoResult<void> {
  return registry_.Register(std::move(event));
}

auto xyco::io::IoRegistry::reregister(std::shared_ptr<runtime::Event> event)
    -> IoResult<void> {
  return registry_.reregister(std::move(event));
}

auto xyco::io::IoRegistry::deregister(std::shared_ptr<runtime::Event> event)
    -> IoResult<void> {
  return registry_.deregister(std::move(event));
}

auto xyco::io::IoRegistry::select(runtime::Events& events,
                                  std::chrono::milliseconds timeout)
    -> IoResult<void> {
  return registry_.select(events, timeout);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::IoExtra>::format(const xyco::io::IoExtra& extra,
                                               FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return fmt::formatter<std::string>::format(extra.print(), ctx);
}

template auto fmt::formatter<xyco::io::IoExtra>::format(
    const xyco::io::IoExtra& extra,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());