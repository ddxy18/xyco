#ifndef XYCO_FS_FILE_H_
#define XYCO_FS_FILE_H_

#include <fcntl.h>
#include <linux/stat.h>

#include <filesystem>

#include "io/utils.h"
#include "runtime/future.h"

namespace fs {
class Permissions {
  friend class File;
  friend class FileAttr;

 public:
  auto set_readonly(bool readonly) -> void;

 private:
  Permissions(mode_t mode);

  mode_t mode_;
};

class FileType {
  friend class FileAttr;

 public:
  [[nodiscard]] auto is_dir() const -> bool;

  [[nodiscard]] auto is_file() const -> bool;

  [[nodiscard]] auto is_symlink() const -> bool;

 private:
  [[nodiscard]] auto is(mode_t mode) const -> bool;

  FileType(mode_t mode);

  mode_t mode_;
};

class StatxExtraFields {
  friend class FileAttr;

 private:
  // This is needed to check if btime is supported by the filesystem.
  uint32_t stx_mask_;
  statx_timestamp stx_btime_;
};

class FileAttr {
 public:
  [[nodiscard]] auto file_type() const -> FileType;

  [[nodiscard]] auto is_dir() const -> bool;

  [[nodiscard]] auto is_file() const -> bool;

  [[nodiscard]] auto is_symlink() const -> bool;

  [[nodiscard]] auto len() const -> uint64_t;

  [[nodiscard]] auto permissions() const -> Permissions;

  auto modified() -> io::IoResult<timespec>;

  auto accessed() -> io::IoResult<timespec>;

  auto created() -> io::IoResult<timespec>;

 private:
  stat64 stat_;
  std::optional<StatxExtraFields> statx_extra_fields_;
};

class File {
  friend class OpenOptions;

 public:
  static auto create(std::filesystem::path path)
      -> runtime::Future<io::IoResult<File>>;

  static auto open(std::filesystem::path path)
      -> runtime::Future<io::IoResult<File>>;

  auto set_len(uint64_t size) -> runtime::Future<io::IoResult<void>>;

  auto metadata() -> runtime::Future<io::IoResult<FileAttr>>;

  [[nodiscard]] auto set_permissions(Permissions permissions) const
      -> runtime::Future<io::IoResult<void>>;

  template <typename Iterator>
  auto read(Iterator begin, Iterator end)
      -> runtime::Future<io::IoResult<uintptr_t>>;

  template <typename Iterator>
  auto write(Iterator begin, Iterator end)
      -> runtime::Future<io::IoResult<uintptr_t>>;

  [[nodiscard]] auto flush() const -> runtime::Future<io::IoResult<void>>;

 private:
  File(int fd) : fd_(fd) {}

  int fd_;
};

class OpenOptions {
 public:
  auto open(std::filesystem::path path) -> runtime::Future<io::IoResult<File>>;

  auto read(bool read) -> OpenOptions &;

  auto write(bool write) -> OpenOptions &;

  auto truncate(bool truncate) -> OpenOptions &;

  auto append(bool append) -> OpenOptions &;

  auto create(bool create) -> OpenOptions &;

  auto create_new(bool create_new) -> OpenOptions &;

  auto custom_flags(int32_t flags) -> OpenOptions &;

  auto mode(uint32_t mode) -> OpenOptions &;

  OpenOptions();

 private:
  [[nodiscard]] auto get_access_mode() const -> io::IoResult<int>;

  [[nodiscard]] auto get_creation_mode() const -> io::IoResult<int>;

  const int default_mode_ = 0666;

  // generic
  bool read_;
  bool write_;
  bool append_;
  bool truncate_;
  bool create_;
  bool create_new_;
  // system-specific
  int32_t custom_flags_;
  mode_t mode_;
};
}  // namespace fs

#endif  // XYCO_FS_FILE_H_