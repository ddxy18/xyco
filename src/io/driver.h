#ifndef XYCO_IO_DRIVER_H_
#define XYCO_IO_DRIVER_H_

#include "epoll/mod.h"
#include "runtime/registry.h"
#include "utils/error.h"

namespace xyco::io {
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

class IoRegistry : public runtime::Registry {
 public:
  [[nodiscard]] auto Register(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto reregister(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto deregister(std::shared_ptr<runtime::Event> event)
      -> utils::Result<void> override;

  [[nodiscard]] auto select(runtime::Events& events,
                            std::chrono::milliseconds timeout)
      -> utils::Result<void> override;

 private:
  io::NetRegistry registry_;
};
}  // namespace xyco::io

template <>
struct fmt::formatter<xyco::io::IoExtra> : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::IoExtra& extra, FormatContext& ctx) const
      -> decltype(ctx.out());
};

#endif  // XYCO_IO_DRIVER_H_