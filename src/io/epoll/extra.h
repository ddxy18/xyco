#ifndef XYCO_IO_EPOLL_EXTRA_H_
#define XYCO_IO_EPOLL_EXTRA_H_

#include "runtime/registry.h"

namespace xyco::io::epoll {
class IoExtra : public runtime::Extra {
 public:
  class State {
    friend class IoExtra;

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
struct fmt::formatter<xyco::io::epoll::IoExtra>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::epoll::IoExtra& extra, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_IO_EPOLL_EXTRA_H_