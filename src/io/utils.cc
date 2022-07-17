#include "utils.h"

template <typename FormatContext>
auto fmt::formatter<xyco::io::Shutdown>::format(
    const xyco::io::Shutdown& shutdown, FormatContext& ctx) const
    -> decltype(ctx.out()) {
  std::string shutdown_type;
  switch (shutdown) {
    case xyco::io::Shutdown::Read:
      shutdown_type = "Read";
      break;
    case xyco::io::Shutdown::Write:
      shutdown_type = "Write";
      break;
    case xyco::io::Shutdown::All:
      shutdown_type = "All";
      break;
  }
  return format_to(ctx.out(), "Shutdown{{{}}}", shutdown_type);
}

template auto fmt::formatter<xyco::io::Shutdown>::format(
    const xyco::io::Shutdown& shutdown,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());