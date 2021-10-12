#include "utils.h"

#include <cerrno>
#include <string>

auto io::IoError::from_sys_error() -> IoError {
  auto err = IoError{};
  err.errno_ = errno;
  return err;
}

auto io::into_sys_result(int return_value) -> io::IoResult<int> {
  if (return_value == -1) {
    return io::IoResult<int>::err(io::IoError::from_sys_error());
  }
  return io::IoResult<int>::ok(return_value);
}

template <typename FormatContext>
auto fmt::formatter<io::IoError>::format(const io::IoError& err,
                                         FormatContext& ctx) const
    -> decltype(ctx.out()) {
  return format_to(ctx.out(), "{{errno={}, info={}}}", err.errno_,
                   fmt::join(err.info_, ""));
}

template auto fmt::formatter<io::IoError>::format(
    const io::IoError& err,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());