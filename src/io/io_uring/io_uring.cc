#include "io_uring.h"

#include "extra.h"
#include "runtime/registry.h"

auto xyco::io::IoUringRegistry::Register(std::shared_ptr<runtime::Event> event)
    -> IoResult<void> {
  auto* sqe = io_uring_get_sqe(&io_uring_);
  if (sqe == nullptr) {  // sq full
    TRACE("submit");
    io_uring_submit(&io_uring_);
    sqe = io_uring_get_sqe(&io_uring_);
  }

  if (sqe != nullptr) {
    registered_events_.push_back(event);
    auto* extra = dynamic_cast<uring::IoExtra*>(event->extra_.get());
    extra->state_.set_field<io::uring::IoExtra::State::Registered>();
    // read
    if (std::holds_alternative<uring::IoExtra::Read>(extra->args_)) {
      auto read_args = std::get<uring::IoExtra::Read>(extra->args_);
      TRACE("read:{} {}", extra->fd_, fmt::ptr(event.get()));

      io_uring_prep_read(sqe, extra->fd_, read_args.buf_, read_args.len_,
                         read_args.offset_);
    }
    // write
    if (std::holds_alternative<uring::IoExtra::Write>(extra->args_)) {
      auto write_args = std::get<uring::IoExtra::Write>(extra->args_);
      TRACE("write:{} {}", extra->fd_, fmt::ptr(event.get()));

      io_uring_prep_write(sqe, extra->fd_, write_args.buf_, write_args.len_,
                          write_args.offset_);
    }
    // close
    if (std::holds_alternative<uring::IoExtra::Close>(extra->args_)) {
      TRACE("close:{} {}", extra->fd_, fmt::ptr(event.get()));

      io_uring_prep_close(sqe, extra->fd_);
    }
    // accept
    if (std::holds_alternative<uring::IoExtra::Accept>(extra->args_)) {
      auto accept_args = std::get<uring::IoExtra::Accept>(extra->args_);
      TRACE("accpet:{} {}", extra->fd_, fmt::ptr(event.get()));

      io_uring_prep_accept(sqe, extra->fd_, accept_args.addr_,
                           accept_args.addrlen_, 0);
    }
    // connect
    if (std::holds_alternative<uring::IoExtra::Connect>(extra->args_)) {
      auto connect_args = std::get<uring::IoExtra::Connect>(extra->args_);
      TRACE("connect:{} {}", extra->fd_, fmt::ptr(event.get()));

      io_uring_prep_connect(sqe, extra->fd_, connect_args.addr_,
                            connect_args.addrlen_);
    }
    // shutdown
    if (std::holds_alternative<uring::IoExtra::Shutdown>(extra->args_)) {
      auto shutdown_args = std::get<uring::IoExtra::Shutdown>(extra->args_);
      TRACE("shutdown:{} {}", extra->fd_, fmt::ptr(event.get()));

      io_uring_prep_shutdown(sqe, extra->fd_,
                             std::__to_underlying(shutdown_args.shutdown_));
    }
    if (!std::holds_alternative<uring::IoExtra::Close>(extra->args_)) {
      io_uring_sqe_set_data(sqe, event.get());
    }
    TRACE("submit");

    io_uring_submit(&io_uring_);

    return IoResult<void>::ok();
  }

  return IoResult<void>::err(IoError{.errno_ = 1, .info_ = ""});
}

auto xyco::io::IoUringRegistry::reregister(
    std::shared_ptr<runtime::Event> event) -> IoResult<void> {
  return IoResult<void>::ok();
}

auto xyco::io::IoUringRegistry::deregister(
    std::shared_ptr<runtime::Event> event) -> IoResult<void> {
  auto* sqe = io_uring_get_sqe(&io_uring_);
  if (sqe == nullptr) {  // sq full
    TRACE("submit");

    io_uring_submit(&io_uring_);
  }

  sqe = io_uring_get_sqe(&io_uring_);
  if (sqe != nullptr) {
    io_uring_prep_cancel(sqe, reinterpret_cast<unsigned long>(event.get()), 0);
    TRACE("cancel:{} {}",
          dynamic_cast<uring::IoExtra*>(event->extra_.get())->fd_,
          fmt::ptr(event.get()));
    TRACE("submit");

    io_uring_submit(&io_uring_);

    registered_events_.erase(
        std::find(registered_events_.begin(), registered_events_.end(), event));
    auto* extra = dynamic_cast<uring::IoExtra*>(event->extra_.get());
    extra->state_.set_field<io::uring::IoExtra::State::Registered, false>();

    return IoResult<void>::ok();
  }

  return IoResult<void>::err(IoError{.errno_ = 1, .info_ = ""});
}

auto xyco::io::IoUringRegistry::select(runtime::Events& events,
                                       std::chrono::milliseconds timeout)
    -> IoResult<void> {
  io_uring_cqe* cqe_ptr = nullptr;
  int return_value = 0;
  __kernel_timespec ts{};
  ts.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(timeout).count();
  ts.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   timeout % std::chrono::seconds(1))
                   .count();
  return_value = io_uring_wait_cqe_timeout(&io_uring_, &cqe_ptr, &ts);
  if (return_value < 0 && (-return_value == ETIME || -return_value == EBUSY)) {
    return IoResult<void>::ok();
  }
  if (return_value < 0) {
    panic();
  }
  if (cqe_ptr != nullptr) {
    unsigned head = 0;
    int count = 0;
    io_uring_for_each_cqe(&io_uring_, head, cqe_ptr) {
      count++;
      if (cqe_ptr->res == 0 && cqe_ptr->user_data == LIBURING_UDATA_TIMEOUT) {
        continue;
      }
      if (io_uring_cqe_get_data(cqe_ptr) == nullptr) {
        continue;
      }
      TRACE("cqe_ptr:{},res:{},flags:{}", fmt::ptr(cqe_ptr), cqe_ptr->res,
            cqe_ptr->flags);

      auto* data = static_cast<runtime::Event*>(io_uring_cqe_get_data(cqe_ptr));
      TRACE("cqe data:{} {}", fmt::ptr(data),
            dynamic_cast<uring::IoExtra*>(data->extra_.get())->fd_);

      dynamic_cast<uring::IoExtra*>(data->extra_.get())->return_ = cqe_ptr->res;
      auto ready_event =
          std::find_if(registered_events_.begin(), registered_events_.end(),
                       [&](auto& registered_event) {
                         return data == registered_event.get();
                       });
      events.push_back(*ready_event);
    }
    io_uring_cq_advance(&io_uring_, count);
    TRACE("advance {}", count);
  }

  return IoResult<void>::ok();
}

xyco::io::IoUringRegistry::IoUringRegistry(uint32_t entries) : io_uring_() {
  auto result = io_uring_queue_init(entries, &io_uring_, 0);
  if (result != 0) {
    panic();
  }
}

xyco::io::IoUringRegistry::~IoUringRegistry() {
  io_uring_queue_exit(&io_uring_);
}