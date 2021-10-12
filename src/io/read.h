#ifndef XYWEBSERVER_EVENT_IO_READ_H_
#define XYWEBSERVER_EVENT_IO_READ_H_

#include <vector>

#include "io/utils.h"
#include "runtime/future.h"

class ReadTrait {
  template <typename T>
  using Future = runtime::Future<T>;

  virtual auto read(std::vector<char> *buf) -> Future<IoResult<uintptr_t>> = 0;
};

#endif  // XYWEBSERVER_EVENT_IO_READ_H_
