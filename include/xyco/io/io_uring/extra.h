#ifndef XYCO_IO_IO_URING_EXTRA_H_
#define XYCO_IO_IO_URING_EXTRA_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <variant>

#include "xyco/io/write.h"
#include "xyco/runtime/registry.h"

import xyco.overload;

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
  int fd_{};
  int return_{};
  State state_{};
};
}  // namespace xyco::io::uring

template <>
struct std::formatter<xyco::io::uring::IoExtra>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra &extra, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return std::format_to(
        ctx.out(), "IoExtra{{args_={}, fd_={}, return_={}}}",
        std::visit(overloaded{[](auto arg) { return std::format("{}", arg); }},
                   extra.args_),
        extra.fd_, extra.return_);
  }
};

template <>
struct std::formatter<xyco::io::uring::IoExtra::Read>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Read &args,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "Read{{len_={}, offset_={}}}", args.len_,
                          args.offset_);
  }
};

template <>
struct std::formatter<xyco::io::uring::IoExtra::Write>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Write &args,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "Write{{len_={}, offset_={}}}", args.len_,
                          args.offset_);
  }
};

template <>
struct std::formatter<xyco::io::uring::IoExtra::Close>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format([[maybe_unused]] const xyco::io::uring::IoExtra::Close &args,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "Close{{}}");
  }
};

template <>
struct std::formatter<xyco::io::uring::IoExtra::Accept>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Accept &args,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    auto *addr = static_cast<sockaddr_in *>(static_cast<void *>(args.addr_));
    std::string ip_addr(INET_ADDRSTRLEN, 0);
    ip_addr = ::inet_ntop(addr->sin_family, &addr->sin_addr, ip_addr.data(),
                          ip_addr.size());
    return std::format_to(ctx.out(), "Accept{{addr_={{{}:{}}}, flags_={}}}",
                          ip_addr, addr->sin_port, args.flags_);
  }
};

template <>
struct std::formatter<xyco::io::uring::IoExtra::Connect>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Connect &args,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    const auto *addr =
        static_cast<const sockaddr_in *>(static_cast<const void *>(args.addr_));
    std::string ip_addr(INET_ADDRSTRLEN, 0);
    ip_addr = ::inet_ntop(addr->sin_family, &addr->sin_addr, ip_addr.data(),
                          ip_addr.size());
    return std::format_to(ctx.out(), "Connect{{addr_={{{}:{}}}}}", ip_addr,
                          addr->sin_port);
  }
};

template <>
struct std::formatter<xyco::io::uring::IoExtra::Shutdown>
    : public std::formatter<std::string> {
  template <typename FormatContext>
  auto format(const xyco::io::uring::IoExtra::Shutdown &args,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "Shutdown{{shutdown_={}}}",
                          args.shutdown_);
  }
};

#endif  // XYCO_IO_IO_URING_EXTRA_H_
