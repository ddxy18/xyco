
#include "xyco/fs/epoll/file.h"

#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include <filesystem>
#include <utility>

#include "xyco/task/blocking_task.h"
#include "xyco/utils/result.h"

class StatxExtraFields {
 public:
  uint32_t stx_mask_;
  statx_timestamp stx_btime_;
};

auto epoll_file_attr(int file_descriptor) -> xyco::runtime::Future<
    xyco::utils::Result<std::pair<struct stat64, StatxExtraFields>>> {
  struct statx stx {};
  struct stat64 stat {};

  ASYNC_TRY((co_await xyco::task::BlockingTask([&]() {
              return xyco::utils::into_sys_result(statx(
                  file_descriptor, "\0", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                  STATX_ALL, &stx));
            })).transform([]([[maybe_unused]] auto n) {
    return std::pair<struct stat64, StatxExtraFields>{{}, StatxExtraFields()};
  }));

  stat.st_dev = makedev(stx.stx_dev_major, stx.stx_dev_minor);
  stat.st_ino = stx.stx_ino;
  stat.st_nlink = stx.stx_nlink;
  stat.st_mode = stx.stx_mode;
  stat.st_uid = stx.stx_uid;
  stat.st_gid = stx.stx_gid;
  stat.st_rdev = makedev(stx.stx_rdev_major, stx.stx_rdev_minor);
  stat.st_size = static_cast<__off_t>(stx.stx_size);
  stat.st_blksize = stx.stx_blksize;
  stat.st_blocks = static_cast<__blkcnt64_t>(stx.stx_blocks);
  stat.st_atim.tv_sec = stx.stx_atime.tv_sec;
  stat.st_atim.tv_nsec = stx.stx_atime.tv_nsec;
  stat.st_mtim.tv_sec = stx.stx_mtime.tv_sec;
  stat.st_mtim.tv_nsec = stx.stx_mtime.tv_nsec;
  stat.st_ctim.tv_sec = stx.stx_ctime.tv_sec;
  stat.st_ctim.tv_nsec = stx.stx_ctime.tv_nsec;

  co_return std::pair{stat, StatxExtraFields{
                                .stx_mask_ = stx.stx_mask,
                                .stx_btime_ = stx.stx_btime,
                            }};
}

auto xyco::fs::epoll::File::modified() const
    -> runtime::Future<utils::Result<timespec>> {
  co_return (co_await epoll_file_attr(fd_)).transform([](auto pair) {
    auto stat = pair.first;
    return timespec{.tv_sec = stat.st_mtim.tv_sec,
                    .tv_nsec = stat.st_mtim.tv_nsec};
  });
}

auto xyco::fs::epoll::File::accessed() const
    -> runtime::Future<utils::Result<timespec>> {
  co_return (co_await epoll_file_attr(fd_)).transform([](auto pair) {
    auto stat = pair.first;
    return timespec{.tv_sec = stat.st_atim.tv_sec,
                    .tv_nsec = stat.st_atim.tv_nsec};
  });
}

auto xyco::fs::epoll::File::created() const
    -> runtime::Future<utils::Result<timespec>> {
  auto result = co_await epoll_file_attr(fd_);
  if (result) {
    auto ext = result->second;
    if ((ext.stx_mask_ & STATX_BTIME) != 0) {
      co_return timespec{.tv_sec = ext.stx_btime_.tv_sec,
                         .tv_nsec = ext.stx_btime_.tv_nsec};
    }
    co_return std::unexpected(utils::Error{
        .errno_ = std::__to_underlying(utils::ErrorKind::Uncategorized),
        .info_ = "creation time is not available for the filesystem"});
  }
  co_return std::unexpected(utils::Error{
      .errno_ = std::__to_underlying(utils::ErrorKind::Unsupported),
      .info_ = "creation time is not available on this platform currently"});
}

auto xyco::fs::epoll::File::create(std::filesystem::path path)
    -> runtime::Future<utils::Result<File>> {
  co_return co_await OpenOptions().write(true).create(true).truncate(true).open(
      std::move(path));
}

auto xyco::fs::epoll::File::open(std::filesystem::path path)
    -> runtime::Future<utils::Result<File>> {
  co_return co_await OpenOptions().read(true).open(std::move(path));
}

auto xyco::fs::epoll::File::resize(uintmax_t size)
    -> runtime::Future<utils::Result<void>> {
  co_return co_await task::BlockingTask([&]() {
    std::error_code error_code;
    std::filesystem::resize_file(path_, size, error_code);
    return !error_code
               ? utils::Result<void>()
               : std::unexpected(utils::Error{.errno_ = error_code.value(),
                                              .info_ = error_code.message()});
  });
}

auto xyco::fs::epoll::File::size() const
    -> runtime::Future<utils::Result<uintmax_t>> {
  co_return co_await task::BlockingTask([&]() {
    std::error_code error_code;
    auto len = std::filesystem::file_size(path_, error_code);
    return !error_code
               ? utils::Result<uintmax_t>(len)
               : std::unexpected(utils::Error{.errno_ = error_code.value(),
                                              .info_ = error_code.message()});
  });
}

auto xyco::fs::epoll::File::status()
    -> runtime::Future<utils::Result<std::filesystem::file_status>> {
  co_return co_await task::BlockingTask([&]() {
    std::error_code error_code;
    std::filesystem::file_status status;
    if (std::filesystem::is_symlink(path_)) {
      status = std::filesystem::symlink_status(path_, error_code);
    } else {
      status = std::filesystem::status(path_, error_code);
    }
    return !error_code
               ? utils::Result<std::filesystem::file_status>(status)
               : std::unexpected(utils::Error{.errno_ = error_code.value(),
                                              .info_ = error_code.message()});
  });
}

auto xyco::fs::epoll::File::set_permissions(
    std::filesystem::perms prms, std::filesystem::perm_options opts) const
    -> runtime::Future<utils::Result<void>> {
  co_return co_await task::BlockingTask([&]() {
    std::error_code error_code;
    std::filesystem::permissions(path_, prms, opts, error_code);
    return !error_code
               ? utils::Result<void>()
               : std::unexpected(utils::Error{.errno_ = error_code.value(),
                                              .info_ = error_code.message()});
  });
}

auto xyco::fs::epoll::File::flush() const
    -> runtime::Future<utils::Result<void>> {
  co_return co_await task::BlockingTask([this]() {
    return utils::into_sys_result(::fsync(fd_))
        .transform([]([[maybe_unused]] auto result) {});
  });
}

auto xyco::fs::epoll::File::seek(off64_t offset, int whence) const
    -> runtime::Future<utils::Result<off64_t>> {
  co_return co_await task::BlockingTask(
      [this, offset, whence]() -> utils::Result<off64_t> {
        auto return_offset = ::lseek64(fd_, offset, whence);
        if (return_offset == -1) {
          return utils::into_sys_result(-1).transform(
              [](auto n) { return static_cast<off64_t>(n); });
        }
        return return_offset;
      });
}

// NOLINTNEXTLINE(bugprone-exception-escape)
xyco::fs::epoll::File::File(File&& file) noexcept { *this = std::move(file); }

// NOLINTNEXTLINE(bugprone-exception-escape)
auto xyco::fs::epoll::File::operator=(File&& file) noexcept -> File& {
  fd_ = file.fd_;
  path_ = file.path_;
  file.fd_ = -1;

  return *this;
}

xyco::fs::epoll::File::~File() {
  if (fd_ != -1) {
    ::close(fd_);
  }
}

xyco::fs::epoll::File::File(int file_descriptor, std::filesystem::path&& path)
    : fd_(file_descriptor), path_(std::move(path)) {}

auto xyco::fs::epoll::OpenOptions::open(std::filesystem::path path)
    -> runtime::Future<utils::Result<File>> {
  co_return co_await task::BlockingTask([&]() {
    auto access_mode = get_access_mode();
    if (!access_mode) {
      return access_mode.transform([]([[maybe_unused]] auto n) {
        return File(-1, std::filesystem::path());
      });
    }
    auto creation_mode = get_creation_mode();
    if (!creation_mode) {
      return creation_mode.transform([]([[maybe_unused]] auto n) {
        return File(-1, std::filesystem::path());
      });
    }

    // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
    int flags = O_CLOEXEC | *access_mode | *creation_mode;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return utils::into_sys_result(::open(path.c_str(), flags, mode_))
        .transform([&](auto file_descriptor) {
          return File(file_descriptor, std::move(path));
        });
  });
}

auto xyco::fs::epoll::OpenOptions::read(bool read) -> OpenOptions& {
  read_ = read;
  return *this;
}

auto xyco::fs::epoll::OpenOptions::write(bool write) -> OpenOptions& {
  write_ = write;
  return *this;
}

auto xyco::fs::epoll::OpenOptions::truncate(bool truncate) -> OpenOptions& {
  truncate_ = truncate;
  return *this;
}

auto xyco::fs::epoll::OpenOptions::append(bool append) -> OpenOptions& {
  append_ = append;
  return *this;
}

auto xyco::fs::epoll::OpenOptions::create(bool create) -> OpenOptions& {
  create_ = create;
  return *this;
}

auto xyco::fs::epoll::OpenOptions::create_new(bool create_new) -> OpenOptions& {
  create_new_ = create_new;
  return *this;
}

auto xyco::fs::epoll::OpenOptions::mode(uint32_t mode) -> OpenOptions& {
  mode_ = mode;
  return *this;
}

xyco::fs::epoll::OpenOptions::OpenOptions() : mode_(default_mode_) {}

auto xyco::fs::epoll::OpenOptions::get_access_mode() const
    -> utils::Result<int> {
  if (read_ && !write_ && !append_) {
    return O_RDONLY;
  }
  if (!read_ && write_ && !append_) {
    return O_WRONLY;
  }
  if (read_ && write_ && !append_) {
    return O_RDWR;
  }
  if (!read_ && append_) {
    return O_WRONLY | O_APPEND;
  }
  if (read_ && append_) {
    return O_RDWR | O_APPEND;
  }
  return std::unexpected(utils::Error{.errno_ = EINVAL});
}

auto xyco::fs::epoll::OpenOptions::get_creation_mode() const
    -> utils::Result<int> {
  if (!write_ && !append_) {
    if (truncate_ || create_ || create_new_) {
      return std::unexpected(utils::Error{.errno_ = EINVAL});
    }
  }
  if (append_) {
    if (truncate_ && !create_new_) {
      return std::unexpected(utils::Error{.errno_ = EINVAL});
    }
  }

  if (!create_ && !truncate_ && !create_new_) {
    return 0;
  }
  if (create_ && !truncate_ && !create_new_) {
    return O_CREAT;
  }
  if (!create_ && truncate_ && !create_new_) {
    return O_TRUNC;
  }
  if (create_ && truncate_ && !create_new_) {
    return O_CREAT | O_TRUNC;
  }
  return O_CREAT | O_EXCL;
}
