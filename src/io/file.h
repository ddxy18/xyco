#ifndef XYWEBSERVER_EVENT_IO_FILE_H_
#define XYWEBSERVER_EVENT_IO_FILE_H_

#include <filesystem>

#include "io/utils.h"

class File;

class File {
 public:
  auto open(std::filesystem::path path) -> IoResult<File>;
  auto create(std::filesystem::path path) -> IoResult<File>;
};

#endif  // XYWEBSERVER_EVENT_IO_FILE_H_