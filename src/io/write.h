#ifndef XYCO_IO_WRITE_H_
#define XYCO_IO_WRITE_H_

#include "io/utils.h"
#include "runtime/future.h"

namespace io {
class WriteTrait {
  template <typename T>
  using Future = runtime::Future<T>;

 public:
  virtual auto write(const std::vector<char> &buf)
      -> Future<IoResult<uintptr_t>> = 0;

  virtual auto write_all(const std::vector<char> &buf)
      -> Future<IoResult<void>> = 0;

  virtual auto flush() -> Future<IoResult<void>> = 0;
};
}  // namespace io

#endif  // XYCO_IO_WRITE_H_