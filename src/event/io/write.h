#ifndef XYWEBSERVER_EVENT_IO_WRITE_H_
#define XYWEBSERVER_EVENT_IO_WRITE_H_

#include "event/io/utils.h"
#include "event/runtime/future.h"

class WriteTrait {
  template <typename T>
  using Future = runtime::Future<T>;

 public:
  virtual auto write(const std::vector<char> &buf)
      -> Future<IoResult<uintptr_t>> = 0;

  virtual auto write_all(const std::vector<char> &buf)
      -> Future<IoResult<Void>> = 0;

  virtual auto flush() -> Future<IoResult<Void>> = 0;
};

#endif  // XYWEBSERVER_EVENT_IO_WRITE_H_