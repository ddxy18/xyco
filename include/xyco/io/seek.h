#ifndef XYCO_IO_WRITE_H_
#define XYCO_IO_WRITE_H_

#include <concepts>

#include "xyco/runtime/future.h"

import xyco.error;

namespace xyco::io {
template <typename Seeker>
concept Seekable = requires(Seeker seeker, off64_t offset, int whence) {
  {
    seeker.seek(offset, whence)
  } -> std::same_as<runtime::Future<utils::Result<off64_t>>>;
};
}  // namespace xyco::io

#endif  // XYCO_IO_WRITE_H_