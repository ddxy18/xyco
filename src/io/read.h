#ifndef XYCO_IO_READ_H_
#define XYCO_IO_READ_H_

#include <vector>

#include "io/utils.h"
#include "runtime/future.h"

namespace io {
class ReadTrait {
  template <typename T>
  using Future = runtime::Future<T>;

  virtual auto read(std::vector<char> *buf) -> Future<IoResult<uintptr_t>> = 0;
};
}  // namespace io

#endif  // XYCO_IO_READ_H_
