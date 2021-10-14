#ifndef XYCO_FS_FILE_H_
#define XYCO_FS_FILE_H_

#include <fcntl.h>
#include <linux/stat.h>

#include <filesystem>

#include "io/utils.h"
#include "runtime/future.h"

namespace fs {
class File {
  friend class OpenOptions;

 public:
  static auto create(std::filesystem::path &&path)
      -> runtime::Future<io::IoResult<File>>;

  static auto open(std::filesystem::path &&path)
      -> runtime::Future<io::IoResult<File>>;

  auto resize(uintmax_t size) -> runtime::Future<io::IoResult<void>>;

  [[nodiscard]] auto size() const -> runtime::Future<io::IoResult<uintmax_t>>;

  auto status() -> runtime::Future<io::IoResult<std::filesystem::file_status>>;

  [[nodiscard]] auto set_permissions(std::filesystem::perms prms,
                                     std::filesystem::perm_options opts =
                                         std::filesystem::perm_options::replace)
      const -> runtime::Future<io::IoResult<void>>;

  [[nodiscard]] auto modified() const
      -> runtime::Future<io::IoResult<timespec>>;

  [[nodiscard]] auto accessed() const
      -> runtime::Future<io::IoResult<timespec>>;

  [[nodiscard]] auto created() const -> runtime::Future<io::IoResult<timespec>>;

  template <typename Iterator>
  auto read(Iterator begin, Iterator end)
      -> runtime::Future<io::IoResult<uintptr_t>>;

  template <typename Iterator>
  auto write(Iterator begin, Iterator end)
      -> runtime::Future<io::IoResult<uintptr_t>>;

  [[nodiscard]] auto flush() const -> runtime::Future<io::IoResult<void>>;

 private:
  File(int fd, std::filesystem::path &&path);

  int fd_;
  std::filesystem::path path_;
};

class OpenOptions {
 public:
  auto open(std::filesystem::path &&path)
      -> runtime::Future<io::IoResult<File>>;

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