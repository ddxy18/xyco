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
