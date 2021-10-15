#include "registry.h"

auto xyco::runtime::IoExtra::readable() const -> bool {
  return state_ == State::Readable || state_ == State::All;
}

auto xyco::runtime::IoExtra::writeable() const -> bool {
  return state_ == State::Writable || state_ == State::All;
}

auto xyco::runtime::IoExtra::clear_readable() -> void {
  if (state_ == State::Readable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Writable;
  }
}

auto xyco::runtime::IoExtra::clear_writeable() -> void {
  if (state_ == State::Writable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Readable;
  }
}

template <typename FormatContext>
auto fmt::formatter<xyco::runtime::Event>::format(
    const xyco::runtime::Event &event, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  if (event.extra_.index() == 0) {
    return format_to(ctx.out(), "Event{{extra_={}}}",
                     std::get<0>(event.extra_));
  }
  return format_to(ctx.out(), "Event{{extra_={}}}", std::get<1>(event.extra_));
}

template <typename FormatContext>
auto fmt::formatter<xyco::runtime::IoExtra>::format(
    const xyco::runtime::IoExtra &extra, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  std::string state;
  switch (extra.state_) {
    case xyco::runtime::IoExtra::State::Pending:
      state = "Pending";
      break;
    case xyco::runtime::IoExtra::State::Readable:
      state = "Readable";
      break;
    case xyco::runtime::IoExtra::State::Writable:
      state = "Writable";
      break;
    case xyco::runtime::IoExtra::State::All:
      state = "All";
      break;
  }

  return format_to(ctx.out(), "IoExtra{{fd_={}, state_={}}}", extra.fd_, state);
}

template <typename FormatContext>
auto fmt::formatter<xyco::runtime::AsyncFutureExtra>::format(
    const xyco::runtime::AsyncFutureExtra &extra, FormatContext &ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "AsyncFutureExtra{{}}");
}

template auto fmt::formatter<xyco::runtime::Event>::format(
    const xyco::runtime::Event &event,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<xyco::runtime::IoExtra>::format(
    const xyco::runtime::IoExtra &extra,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());

template auto fmt::formatter<xyco::runtime::AsyncFutureExtra>::format(
    const xyco::runtime::AsyncFutureExtra &extra,
    fmt::basic_format_context<fmt::appender, char> &ctx) const
    -> decltype(ctx.out());