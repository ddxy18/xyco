module;

#include <__utility/to_underlying.h>

#include <cerrno>
#include <expected>

module xyco.error;

auto xyco::utils::Error::from_sys_error() -> Error {
  auto err = Error{};
  err.errno_ = errno;
  return err;
}

auto xyco::utils::into_sys_result(int return_value) -> utils::Result<int> {
  if (return_value == -1) {
    return std::unexpected(utils::Error::from_sys_error());
  }
  return return_value;
}
