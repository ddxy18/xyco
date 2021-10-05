#include "utils.h"

#include <cerrno>
#include <string>

auto IoError::from_sys_error() -> IoError {
  auto err = IoError{};
  err.errno_ = errno;
  return err;
}

auto into_sys_result(int return_value) -> IoResult<int> {
  if (return_value == -1) {
    return err<int, IoError>(IoError::from_sys_error());
  }
  return ok<int, IoError>(return_value);
}

template <typename FormatContext>
auto fmt::formatter<IoError>::format(const IoError& err,
                                     FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "{{errno={}, info={}}}", err.errno_,
                   fmt::join(err.info_, ""));
}

template auto fmt::formatter<IoError>::format(
    const IoError& err,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());