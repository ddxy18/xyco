#include "extra.h"

auto xyco::io::epoll::IoExtra::print() const -> std::string {
  std::string state = "[";
  if ((state_.field_ & xyco::io::epoll::IoExtra::State::Registered) != 0) {
    state += "Registered,";
  }
  if ((state_.field_ & xyco::io::epoll::IoExtra::State::Pending) != 0) {
    state += "Pending,";
  }
  if ((state_.field_ & xyco::io::epoll::IoExtra::State::Readable) != 0) {
    state += "Readable,";
  }
  if ((state_.field_ & xyco::io::epoll::IoExtra::State::Writable) != 0) {
    state += "Writable,";
  }
  if ((state_.field_ & xyco::io::epoll::IoExtra::State::Error) != 0) {
    state += "Error,";
  }
  state[state.length() - 1] = ']';

  std::string interest;
  switch (interest_) {
    case xyco::io::epoll::IoExtra::Interest::Read:
      interest = "Read";
      break;
    case xyco::io::epoll::IoExtra::Interest::Write:
      interest = "Write";
      break;
    case xyco::io::epoll::IoExtra::Interest::All:
      interest = "All";
      break;
  }

  return fmt::format("IoExtra{{state_={}, interest_={}, fd_={}}}", state,
                     interest, fd_);
}

xyco::io::epoll::IoExtra::IoExtra(Interest interest, int file_descriptor)
    : state_(), interest_(interest), fd_(file_descriptor) {}

template <typename FormatContext>
auto fmt::formatter<xyco::io::epoll::IoExtra>::format(
    const xyco::io::epoll::IoExtra& extra, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return fmt::formatter<std::string>::format(extra.print(), ctx);
}

template auto fmt::formatter<xyco::io::epoll::IoExtra>::format(
    const xyco::io::epoll::IoExtra& extra,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());