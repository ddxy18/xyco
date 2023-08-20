#ifndef XYCO_FS_EPOLL_FILE_H_
#define XYCO_FS_EPOLL_FILE_H_

#include <fcntl.h>
#include <linux/stat.h>
#include <unistd.h>

#include <filesystem>

#include "xyco/io/epoll/extra.h"
#include "xyco/io/epoll/registry.h"
#include "xyco/runtime/runtime_ctx.h"
#include "xyco/task/blocking_task.h"
#include "xyco/utils/error.h"

namespace xyco::fs::epoll {
class File {
  friend class OpenOptions;
  friend struct std::formatter<File>;

 public:
  static auto create(std::filesystem::path path)
      -> runtime::Future<utils::Result<File>>;

  static auto open(std::filesystem::path path)
      -> runtime::Future<utils::Result<File>>;

  auto resize(uintmax_t size) -> runtime::Future<utils::Result<void>>;

  [[nodiscard]] auto size() const -> runtime::Future<utils::Result<uintmax_t>>;

  auto status() -> runtime::Future<utils::Result<std::filesystem::file_status>>;

  [[nodiscard]] auto set_permissions(std::filesystem::perms prms,
                                     std::filesystem::perm_options opts =
                                         std::filesystem::perm_options::replace)
      const -> runtime::Future<utils::Result<void>>;

  [[nodiscard]] auto modified() const
      -> runtime::Future<utils::Result<timespec>>;

  [[nodiscard]] auto accessed() const
      -> runtime::Future<utils::Result<timespec>>;

  [[nodiscard]] auto created() const
      -> runtime::Future<utils::Result<timespec>>;

  template <typename Iterator>
  auto read(Iterator begin, Iterator end)
      -> runtime::Future<utils::Result<uintptr_t>> {
    co_return co_await task::BlockingTask([&]() {
      return utils::into_sys_result(
          ::read(fd_, &*begin, std::distance(begin, end)));
    });
  }

  template <typename Iterator>
  auto write(Iterator begin, Iterator end)
      -> runtime::Future<utils::Result<uintptr_t>> {
    co_return co_await task::BlockingTask([&]() {
      return utils::into_sys_result(
          ::write(fd_, &*begin, std::distance(begin, end)));
    });
  }

  [[nodiscard]] auto flush() const -> runtime::Future<utils::Result<void>>;

  [[nodiscard]] auto seek(off64_t offset, int whence) const
      -> runtime::Future<utils::Result<off64_t>>;

  File(const File &file) = delete;

  File(File &&file) noexcept;

  auto operator=(const File &file) = delete;

  auto operator=(File &&file) noexcept -> File &;

  ~File();

 private:
  File(int file_descriptor, std::filesystem::path &&path);

  int fd_;
  std::filesystem::path path_;
};

class OpenOptions {
 public:
  auto open(std::filesystem::path path) -> runtime::Future<utils::Result<File>>;

  auto read(bool read) -> OpenOptions &;

  auto write(bool write) -> OpenOptions &;

  auto truncate(bool truncate) -> OpenOptions &;

  auto append(bool append) -> OpenOptions &;

  auto create(bool create) -> OpenOptions &;

  auto create_new(bool create_new) -> OpenOptions &;

  auto mode(uint32_t mode) -> OpenOptions &;

  OpenOptions();

 private:
  [[nodiscard]] auto get_access_mode() const -> utils::Result<int>;

  [[nodiscard]] auto get_creation_mode() const -> utils::Result<int>;

  static constexpr int default_mode_ = 0666;

  // generic
  bool read_;
  bool write_;
  bool append_;
  bool truncate_;
  bool create_;
  bool create_new_;
  // system-specific
  mode_t mode_;
};
}  // namespace xyco::fs::epoll

template <>
struct std::formatter<xyco::fs::epoll::File> : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::fs::epoll::File &file, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "File{{path_={}}}", file.path_.c_str());
  }
};

#endif  // XYCO_FS_EPOLL_FILE_H_