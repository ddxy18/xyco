#ifndef XYCO_IO_FILE_H_
#define XYCO_IO_FILE_H_

#include <filesystem>

#include "io/utils.h"

namespace xyco::io {
class File {
 public:
  auto open(std::filesystem::path path) -> IoResult<File>;
  auto create(std::filesystem::path path) -> IoResult<File>;
};
}  // namespace xyco::io

#endif  // XYCO_IO_FILE_H_