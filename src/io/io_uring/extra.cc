#include "extra.h"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <variant>

#include "utils/logger.h"
#include "utils/overload.h"

auto xyco::io::uring::IoExtra::print() const -> std::string {
  return fmt::format(
      "IoExtra{{args_={}, fd_={}, return_={}}}",
      std::visit(overloaded{[](auto arg) { return fmt::format("{}", arg); }},
                 args_),
      fd_, return_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::uring::IoExtra>::format(
    const xyco::io::uring::IoExtra& extra, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return fmt::formatter<std::string>::format(extra.print(), ctx);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::uring::IoExtra::Read>::format(
    const xyco::io::uring::IoExtra::Read& args, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "Read{{len_={}, offset_={}}}", args.len_,
                   args.offset_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::uring::IoExtra::Write>::format(
    const xyco::io::uring::IoExtra::Write& args, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "Write{{len_={}, offset_={}}}", args.len_,
                   args.offset_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::uring::IoExtra::Close>::format(
    const xyco::io::uring::IoExtra::Close& args, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "Close{{}}");
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::uring::IoExtra::Accept>::format(
    const xyco::io::uring::IoExtra::Accept& args, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  auto* addr = static_cast<sockaddr_in*>(static_cast<void*>(args.addr_));
  std::string ip_addr(INET_ADDRSTRLEN, 0);
  ::inet_ntop(addr->sin_family, &addr->sin_addr, ip_addr.data(),
              ip_addr.size());
  return format_to(ctx.out(), "Accept{{addr_={{{}:{}}}, flags_={}}}",
                   ip_addr.c_str(), addr->sin_port, args.flags_);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::uring::IoExtra::Connect>::format(
    const xyco::io::uring::IoExtra::Connect& args, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  const auto* addr =
      static_cast<const sockaddr_in*>(static_cast<const void*>(args.addr_));
  std::string ip_addr(INET_ADDRSTRLEN, 0);
  ::inet_ntop(addr->sin_family, &addr->sin_addr, ip_addr.data(),
              ip_addr.size());
  return format_to(ctx.out(), "Connect{{addr_={{{}:{}}}}}", ip_addr.c_str(),
                   addr->sin_port);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::uring::IoExtra::Shutdown>::format(
    const xyco::io::uring::IoExtra::Shutdown& args, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "Shutdown{{shutdown_={}}}", args.shutdown_);
}

template auto fmt::formatter<xyco::io::uring::IoExtra>::format(
    const xyco::io::uring::IoExtra& extra,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());