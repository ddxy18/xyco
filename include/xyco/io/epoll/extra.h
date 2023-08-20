#ifndef XYCO_IO_EPOLL_EXTRA_H_
#define XYCO_IO_EPOLL_EXTRA_H_

#include "xyco/runtime/registry.h"

namespace xyco::io::epoll {
class IoExtra : public runtime::Extra {
 public:
  class State {
    friend struct std::formatter<xyco::io::epoll::IoExtra>;

   public:
    enum {
      Registered = 0b1,
      Pending = 0b10,
      Readable = 0b100,
      Writable = 0b1000,
      Error = 0b10000
    };

    template <size_t F, bool flag = true>
    auto set_field() {
      if constexpr (flag) {
        field_ |= F;
      } else {
        field_ &= ~F;
      }
    }

    template <size_t F>
    [[nodiscard]] auto get_field() -> bool {
      return field_ & F;
    }

   private:
    uint8_t field_;
  };
  enum class Interest { Read, Write, All };

  [[nodiscard]] auto print() const -> std::string override;

  IoExtra(Interest interest, int file_descriptor);

  State state_;
  Interest interest_;
  int fd_;
};
}  // namespace xyco::io::epoll

template <>
struct std::formatter<xyco::io::epoll::IoExtra>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::epoll::IoExtra& extra, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    std::string state = "[";
    if ((extra.state_.field_ & xyco::io::epoll::IoExtra::State::Registered) !=
        0) {
      state += "Registered,";
    }
    if ((extra.state_.field_ & xyco::io::epoll::IoExtra::State::Pending) != 0) {
      state += "Pending,";
    }
    if ((extra.state_.field_ & xyco::io::epoll::IoExtra::State::Readable) !=
        0) {
      state += "Readable,";
    }
    if ((extra.state_.field_ & xyco::io::epoll::IoExtra::State::Writable) !=
        0) {
      state += "Writable,";
    }
    if ((extra.state_.field_ & xyco::io::epoll::IoExtra::State::Error) != 0) {
      state += "Error,";
    }
    state[state.length() - 1] = ']';

    std::string interest;
    switch (extra.interest_) {
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

    return std::format_to(ctx.out(),
                          "IoExtra{{state_={}, interest_={}, fd_={}}}", state,
                          interest, extra.fd_);
  }
};

#endif  // XYCO_IO_EPOLL_EXTRA_H_