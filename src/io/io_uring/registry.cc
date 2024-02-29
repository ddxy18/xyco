module;

#include <liburing.h>

#include <expected>
#include <format>
#include <string>
#include <variant>

module xyco.io.uring;

import xyco.logging;
import xyco.panic;

auto xyco::io::uring::IoExtra::print() const -> std::string { return std::format("{}", *this); }

auto xyco::io::uring::IoRegistryImpl::Register(std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  auto* sqe = io_uring_get_sqe(&io_uring_);
  if (sqe == nullptr) {  // sq full
    io_uring_submit(&io_uring_);
    sqe = io_uring_get_sqe(&io_uring_);
  }

  if (sqe != nullptr) {
    auto* extra = dynamic_cast<uring::IoExtra*>(event->extra_.get());
    if (!extra->state_.get_field<IoExtra::State::Registered>()) {
      registered_events_.push_back(event);
      extra->state_.set_field<io::uring::IoExtra::State::Registered>();
    }
    // read
    if (std::holds_alternative<uring::IoExtra::Read>(extra->args_)) {
      auto read_args = std::get<uring::IoExtra::Read>(extra->args_);
      logging::trace("read:fd={} user_data={}", extra->fd_, static_cast<void*>(event.get()));

      io_uring_prep_read(sqe, extra->fd_, read_args.buf_, read_args.len_, read_args.offset_);
    }
    // write
    if (std::holds_alternative<uring::IoExtra::Write>(extra->args_)) {
      auto write_args = std::get<uring::IoExtra::Write>(extra->args_);
      logging::trace("write:fd={} user_data={}", extra->fd_, static_cast<void*>(event.get()));

      io_uring_prep_write(sqe, extra->fd_, write_args.buf_, write_args.len_, write_args.offset_);
    }
    // close
    if (std::holds_alternative<uring::IoExtra::Close>(extra->args_)) {
      logging::trace("close:fd={} user_data={}", extra->fd_, static_cast<void*>(event.get()));

      io_uring_prep_close(sqe, extra->fd_);
    }
    // accept
    if (std::holds_alternative<uring::IoExtra::Accept>(extra->args_)) {
      auto accept_args = std::get<uring::IoExtra::Accept>(extra->args_);
      logging::trace("accpet:fd={} user_data={}", extra->fd_, static_cast<void*>(event.get()));

      io_uring_prep_accept(sqe, extra->fd_, accept_args.addr_, accept_args.addrlen_, 0);
    }
    // connect
    if (std::holds_alternative<uring::IoExtra::Connect>(extra->args_)) {
      auto connect_args = std::get<uring::IoExtra::Connect>(extra->args_);
      logging::trace("connect:fd={} user_data={}", extra->fd_, static_cast<void*>(event.get()));

      io_uring_prep_connect(sqe, extra->fd_, connect_args.addr_, connect_args.addrlen_);
    }
    // shutdown
    if (std::holds_alternative<uring::IoExtra::Shutdown>(extra->args_)) {
      auto shutdown_args = std::get<uring::IoExtra::Shutdown>(extra->args_);
      logging::trace("shutdown:fd={} user_data={}", extra->fd_, static_cast<void*>(event.get()));

      io_uring_prep_shutdown(sqe, extra->fd_, std::to_underlying(shutdown_args.shutdown_));
    }
    // `user_data` must be set after calling `io_uring_prep_xxx` since
    // `io_uring_prep_xxx` clears `user_data`.
    io_uring_sqe_set_data(sqe, event.get());

    io_uring_submit(&io_uring_);

    return {};
  }

  return std::unexpected(utils::Error{.errno_ = 1, .info_ = ""});
}

auto xyco::io::uring::IoRegistryImpl::reregister(
    [[maybe_unused]] std::shared_ptr<runtime::Event> event) -> utils::Result<void> {
  return {};
}

auto xyco::io::uring::IoRegistryImpl::deregister(std::shared_ptr<runtime::Event> event)
    -> utils::Result<void> {
  auto* sqe = io_uring_get_sqe(&io_uring_);
  if (sqe == nullptr) {  // sq full
    io_uring_submit(&io_uring_);
  }

  sqe = io_uring_get_sqe(&io_uring_);
  if (sqe != nullptr) {
    io_uring_prep_cancel(sqe, event.get(), 0);
    logging::trace("cancel:{} {}", dynamic_cast<uring::IoExtra*>(event->extra_.get())->fd_,
                   static_cast<void*>(event.get()));

    io_uring_submit(&io_uring_);

    registered_events_.erase(
        std::find(registered_events_.begin(), registered_events_.end(), event));
    auto* extra = dynamic_cast<uring::IoExtra*>(event->extra_.get());
    extra->state_.set_field<io::uring::IoExtra::State::Registered, false>();

    return {};
  }

  return std::unexpected(utils::Error{.errno_ = 1, .info_ = ""});
}

xyco::io::uring::IoRegistryImpl::IoRegistryImpl(uint32_t entries) : io_uring_() {
  auto result = io_uring_queue_init(entries, &io_uring_, 0);
  if (result != 0) {
    utils::panic();
  }
}

xyco::io::uring::IoRegistryImpl::~IoRegistryImpl() { io_uring_queue_exit(&io_uring_); }
