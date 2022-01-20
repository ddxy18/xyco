#include "driver.h"

#include "io/utils.h"
#include "net/driver/epoll.h"
#include "runtime/future.h"
#include "runtime/runtime.h"

auto xyco::io::IoExtra::readable() const -> bool {
  return state_ == State::Readable || state_ == State::All;
}

auto xyco::io::IoExtra::writeable() const -> bool {
  return state_ == State::Writable || state_ == State::All;
}

auto xyco::io::IoExtra::clear_readable() -> void {
  if (state_ == State::Readable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Writable;
  }
}

auto xyco::io::IoExtra::clear_writeable() -> void {
  if (state_ == State::Writable) {
    state_ = State::Pending;
  }
  if (state_ == State::All) {
    state_ = State::Readable;
  }
}

auto xyco::io::IoExtra::print() const -> std::string {
  std::string state;
  switch (state_) {
    case xyco::io::IoExtra::State::Pending:
      state = "Pending";
      break;
    case xyco::io::IoExtra::State::Readable:
      state = "Readable";
      break;
    case xyco::io::IoExtra::State::Writable:
      state = "Writable";
      break;
    case xyco::io::IoExtra::State::All:
      state = "All";
      break;
  }

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

xyco::io::IoExtra::IoExtra(Interest interest, int fd, State state)
    : state_(state), interest_(interest), fd_(fd) {}

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
  return runtime::RuntimeCtx::get_ctx()
      ->driver()
      .local_handle<net::NetRegistry>()
      ->Register(ev);
}

auto xyco::io::IoRegistry::reregister_local(runtime::Event& ev)
    -> IoResult<void> {
  return runtime::RuntimeCtx::get_ctx()
      ->driver()
      .local_handle<net::NetRegistry>()
      ->reregister(ev);
}

auto xyco::io::IoRegistry::deregister_local(runtime::Event& ev)
    -> IoResult<void> {
  return runtime::RuntimeCtx::get_ctx()
      ->driver()
      .local_handle<net::NetRegistry>()
      ->deregister(ev);
}

auto xyco::io::IoRegistry::select(runtime::Events& events,
                                  std::chrono::milliseconds timeout)
    -> IoResult<void> {
  return IoResult<void>::ok();
}

auto xyco::io::IoRegistry::local_registry_init() -> void {
  runtime::RuntimeCtx::get_ctx()
      ->driver()
      .add_local_registry<net::NetRegistry>();
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