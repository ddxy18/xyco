#include "utils.h"

#include <__utility/to_underlying.h>

auto xyco::io::IoError::from_sys_error() -> IoError {
  auto err = IoError{};
  err.errno_ = errno;
  return err;
}

auto xyco::io::into_sys_result(int return_value) -> io::IoResult<int> {
  if (return_value == -1) {
    return io::IoResult<int>::err(io::IoError::from_sys_error());
  }
  return io::IoResult<int>::ok(return_value);
}

template <typename FormatContext>
auto fmt::formatter<xyco::io::IoError>::format(const xyco::io::IoError& err,
                                               FormatContext& ctx) const
    -> decltype(ctx.out()) {
  if (err.errno_ > 0) {
    return format_to(ctx.out(), "IoError{{errno={}, info={}}}", err.errno_,
                     fmt::join(err.info_, ""));
  }
  std::string error_kind;
  switch (err.errno_) {
    case std::__to_underlying(xyco::io::ErrorKind::Uncategorized):
      error_kind = std::string("Uncategorized");
    case std::__to_underlying(xyco::io::ErrorKind::Unsupported):
      error_kind = std::string("Unsupported");
  }
  return format_to(ctx.out(), "IoError{{error_kind={}, info={}}}", error_kind,
                   fmt::join(err.info_, ""));
}

template auto fmt::formatter<xyco::io::IoError>::format(
    const xyco::io::IoError& err,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());