#ifndef XYCO_IO_IO_URING_EXTRA_H_
#define XYCO_IO_IO_URING_EXTRA_H_

#include <sys/socket.h>

#include "io/write.h"
#include "runtime/registry.h"

namespace xyco::io::uring {
class IoExtra : public runtime::Extra {
 public:
  class Read {
   public:
    void *buf_;
    unsigned int len_;
    uint64_t offset_;
  };
  class Write {
   public:
    const void *buf_;
    unsigned int len_;
    uint64_t offset_;
  };
  class Close {};
  class Accept {
   public:
    sockaddr *addr_;
    socklen_t *addrlen_;
    int flags_;
  };
  class Connect {
   public:
    const sockaddr *addr_;
    socklen_t addrlen_;
  };
  class Shutdown {
   public:
    io::Shutdown shutdown_;
  };

  class State {
    friend class IoExtra;

   public:
    enum { Registered = 0b1, Completed = 0b10 };

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

  [[nodiscard]] auto print() const -> std::string override;

  std::variant<Read, Write, Close, Accept, Connect, Shutdown> args_;
  int fd_;
  int return_;
  State state_;
};
}  // namespace xyco::io::uring

template <>
struct fmt::formatter<xyco::io::uring::IoExtra>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra &extra, FormatContext &ctx) const
      -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::io::uring::IoExtra::Read>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Read &args,
              FormatContext &ctx) const -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::io::uring::IoExtra::Write>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Write &args,
              FormatContext &ctx) const -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::io::uring::IoExtra::Close>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Close &args,
              FormatContext &ctx) const -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::io::uring::IoExtra::Accept>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Accept &args,
              FormatContext &ctx) const -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::io::uring::IoExtra::Connect>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Connect &args,
              FormatContext &ctx) const -> decltype(ctx.out());
};

template <>
struct fmt::formatter<xyco::io::uring::IoExtra::Shutdown>
    : public fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Shutdown &args,
              FormatContext &ctx) const -> decltype(ctx.out());
};

#endif  // XYCO_IO_IO_URING_EXTRA_H_