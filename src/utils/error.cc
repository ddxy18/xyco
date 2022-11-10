#include "error.h"

#include <__utility/to_underlying.h>

auto xyco::utils::Error::from_sys_error() -> Error {
  auto err = Error{};
  err.errno_ = errno;
  return err;
}

auto xyco::utils::into_sys_result(int return_value) -> utils::Result<int> {
  if (return_value == -1) {
    return utils::Result<int>::err(utils::Error::from_sys_error());
  }
  return utils::Result<int>::ok(return_value);
}

template <typename FormatContext>
auto fmt::formatter<xyco::utils::Error>::format(const xyco::utils::Error& err,
                                                FormatContext& ctx) const
    -> decltype(ctx.out()) {
  if (err.errno_ > 0) {
    return fmt::format_to(ctx.out(), "IoError{{errno={}, info={}}}", err.errno_,
                          fmt::join(err.info_, ""));
  }
  std::string error_kind;
  switch (err.errno_) {
    case std::__to_underlying(xyco::utils::ErrorKind::Uncategorized):
      error_kind = std::string("Uncategorized");
      break;
    case std::__to_underlying(xyco::utils::ErrorKind::Unsupported):
      error_kind = std::string("Unsupported");
  }
  return fmt::format_to(ctx.out(), "IoError{{error_kind={}, info={}}}",
                        error_kind, fmt::join(err.info_, ""));
}

template auto fmt::formatter<xyco::utils::Error>::format(
    const xyco::utils::Error& err,
    fmt::basic_format_context<fmt::appender, char>& ctx) const
    -> decltype(ctx.out());