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
    return Err<int, IoError>(IoError::from_sys_error());
  }
  return Ok<int, IoError>(return_value);
}
