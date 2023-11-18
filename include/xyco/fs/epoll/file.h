#ifndef XYCO_FS_EPOLL_FILE_H_
#define XYCO_FS_EPOLL_FILE_H_

#include <fcntl.h>
#include <linux/stat.h>
#include <unistd.h>

#include <filesystem>

#include "xyco/fs/file_common.h"
#include "xyco/io/epoll/extra.h"
#include "xyco/io/epoll/registry.h"
#include "xyco/runtime/runtime_ctx.h"
#include "xyco/task/blocking_task.h"

import xyco.error;

namespace xyco::fs::epoll {
class OpenOptions;

class File : public FileBase<File> {
  friend class OpenOptions;

 public:
  using OpenOptions = OpenOptions;

  [[nodiscard]] auto size() const -> runtime::Future<utils::Result<uintmax_t>>;

  auto status() -> runtime::Future<utils::Result<std::filesystem::file_status>>;

  [[nodiscard]] auto set_permissions(std::filesystem::perms prms,
                                     std::filesystem::perm_options opts =
                                         std::filesystem::perm_options::replace)
      -> runtime::Future<utils::Result<void>>;

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

 private:
  File(int file_descriptor, std::filesystem::path &&path);
};

class OpenOptions : public OpenOptionsBase<OpenOptions> {
 public:
  auto open(std::filesystem::path path) -> runtime::Future<utils::Result<File>>;
};
}  // namespace xyco::fs::epoll

#endif  // XYCO_FS_EPOLL_FILE_H_
