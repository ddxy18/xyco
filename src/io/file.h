#ifndef XYCO_IO_FILE_H_
#define XYCO_IO_FILE_H_

#include <filesystem>

#include "utils/error.h"

namespace xyco::io {
class File {
 public:
  auto open(std::filesystem::path path) -> utils::Result<File>;
  auto create(std::filesystem::path path) -> utils::Result<File>;
};
}  // namespace xyco::io

#endif  // XYCO_IO_FILE_H_