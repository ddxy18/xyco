
#include "file.h"

#include <__utility/to_underlying.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include <filesystem>
#include <utility>

#include "runtime/async_future.h"
#include "utils/result.h"

class StatxExtraFields {
 public:
  uint32_t stx_mask_;
  statx_timestamp stx_btime_;
};

auto file_attr(int fd) -> runtime::Future<
    io::IoResult<std::pair<struct stat64, StatxExtraFields>>> {
  struct statx stx {};
  struct stat64 stat {};

  ASYNC_TRY((co_await runtime::AsyncFuture<io::IoResult<int>>([&]() {
              return io::into_sys_result(
                  statx(fd, "\0", AT_EMPTY_PATH | AT_STATX_SYNC_AS_STAT,
                        STATX_ALL, &stx));
            })).map([](auto n) {
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

  co_return io::IoResult<std::pair<struct stat64, StatxExtraFields>>::ok(
      stat, StatxExtraFields{
                .stx_mask_ = stx.stx_mask,
                .stx_btime_ = stx.stx_btime,
            });
}

auto fs::File::modified() const -> runtime::Future<io::IoResult<timespec>> {
  co_return(co_await file_attr(fd_)).map([](auto pair) {
    auto stat = pair.first;
    return timespec{.tv_sec = stat.st_mtim.tv_sec,
                    .tv_nsec = stat.st_mtim.tv_nsec};
  });
}

auto fs::File::accessed() const -> runtime::Future<io::IoResult<timespec>> {
  co_return(co_await file_attr(fd_)).map([](auto pair) {
    auto stat = pair.first;
    return timespec{.tv_sec = stat.st_atim.tv_sec,
                    .tv_nsec = stat.st_atim.tv_nsec};
  });
}

auto fs::File::created() const -> runtime::Future<io::IoResult<timespec>> {
  auto result = co_await file_attr(fd_);
  if (result.is_ok()) {
    auto ext = result.unwrap().second;
    if ((ext.stx_mask_ & STATX_BTIME) != 0) {
      co_return io::IoResult<timespec>::ok(timespec{
          .tv_sec = ext.stx_btime_.tv_sec, .tv_nsec = ext.stx_btime_.tv_nsec});
    }
    co_return io::IoResult<timespec>::err(io::IoError{
        .errno_ = std::__to_underlying(io::ErrorKind::Uncategorized),
        .info_ = "creation time is not available for the filesystem"});
  }
  co_return io::IoResult<timespec>::err(io::IoError{
      .errno_ = std::__to_underlying(io::ErrorKind::Unsupported),
      .info_ = "creation time is not available on this platform currently"});
}

auto fs::File::create(std::filesystem::path&& path)
    -> runtime::Future<io::IoResult<File>> {
  co_return co_await OpenOptions().write(true).create(true).truncate(true).open(
      std::move(path));
}

auto fs::File::open(std::filesystem::path&& path)
    -> runtime::Future<io::IoResult<File>> {
  co_return co_await OpenOptions().read(true).open(std::move(path));
}

auto fs::File::resize(uintmax_t size) -> runtime::Future<io::IoResult<void>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<void>>([&]() {
    std::error_code error_code;
    std::filesystem::resize_file(path_, size, error_code);
    if (error_code) {
      return io::IoResult<void>::ok();
    }
    return io::IoResult<void>::err(io::IoError{.errno_ = error_code.value(),
                                               .info_ = error_code.message()});
  });
}

auto fs::File::size() const -> runtime::Future<io::IoResult<uintmax_t>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<uintmax_t>>([&]() {
    std::error_code error_code;
    auto len = std::filesystem::file_size(path_, error_code);
    if (error_code) {
      return io::IoResult<uintmax_t>::ok(len);
    }
    return io::IoResult<uintmax_t>::err(io::IoError{
        .errno_ = error_code.value(), .info_ = error_code.message()});
  });
}

auto fs::File::status()
    -> runtime::Future<io::IoResult<std::filesystem::file_status>> {
  co_return co_await runtime::AsyncFuture<
      io::IoResult<std::filesystem::file_status>>([&]() {
    std::error_code error_code;
    std::filesystem::file_status status;
    if (std::filesystem::is_symlink(path_)) {
      status = std::filesystem::symlink_status(path_, error_code);
    } else {
      status = std::filesystem::status(path_, error_code);
    }
    if (error_code) {
      return io::IoResult<std::filesystem::file_status>::ok(status);
    }
    return io::IoResult<std::filesystem::file_status>::err(io::IoError{
        .errno_ = error_code.value(), .info_ = error_code.message()});
  });
}

auto fs::File::set_permissions(std::filesystem::perms prms,
                               std::filesystem::perm_options opts) const
    -> runtime::Future<io::IoResult<void>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<void>>([&]() {
    std::error_code error_code;
    std::filesystem::permissions(path_, prms, opts, error_code);
    if (error_code) {
      return io::IoResult<void>::ok();
    }
    return io::IoResult<void>::err(io::IoError{.errno_ = error_code.value(),
                                               .info_ = error_code.message()});
  });
}

template <typename Iterator>
auto fs::File::read(Iterator begin, Iterator end)
    -> runtime::Future<io::IoResult<uintptr_t>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<uintptr_t>>([&]() {
    return io::into_sys_result(::read(fd_, &*begin, std::distance(begin, end)));
  });
}

template <typename Iterator>
auto fs::File::write(Iterator begin, Iterator end)
    -> runtime::Future<io::IoResult<uintptr_t>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<uintptr_t>>([&]() {
    return io::into_sys_result(
        ::write(fd_, &*begin, std::distance(begin, end)));
  });
}

auto fs::File::flush() const -> runtime::Future<io::IoResult<void>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<void>>(
      [=]() { return io::into_sys_result(::fsync(fd_)); });
}

fs::File::File(int fd, std::filesystem::path&& path)
    : fd_(fd), path_(std::move(path)) {}

auto fs::OpenOptions::open(std::filesystem::path&& path)
    -> runtime::Future<io::IoResult<File>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<File>>([&]() {
    auto access_mode = get_access_mode();
    if (access_mode.is_err()) {
      return access_mode.map(
          [](auto n) { return File(-1, std::filesystem::path()); });
    }
    auto creation_mode = get_creation_mode();
    if (creation_mode.is_err()) {
      return creation_mode.map(
          [](auto n) { return File(-1, std::filesystem::path()); });
    }

    int flags = O_CLOEXEC | access_mode.unwrap() | creation_mode.unwrap() |
                (custom_flags_ & O_ACCMODE);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return io::into_sys_result(::open(path.c_str(), flags, mode_))
        .map([&](auto fd) { return File(fd, std::move(path)); });
  });
}

auto fs::OpenOptions::read(bool read) -> OpenOptions& {
  read_ = read;
  return *this;
}

auto fs::OpenOptions::write(bool write) -> OpenOptions& {
  write_ = write;
  return *this;
}

auto fs::OpenOptions::truncate(bool truncate) -> OpenOptions& {
  truncate_ = truncate;
  return *this;
}

auto fs::OpenOptions::append(bool append) -> OpenOptions& {
  append_ = true;
  return *this;
}

auto fs::OpenOptions::create(bool create) -> OpenOptions& {
  create_ = create;
  return *this;
}

auto fs::OpenOptions::create_new(bool create_new) -> OpenOptions& {
  create_new_ = create_new;
  return *this;
}

auto fs::OpenOptions::custom_flags(int32_t flags) -> OpenOptions& {
  custom_flags_ = flags;
  return *this;
}

auto fs::OpenOptions::mode(uint32_t mode) -> OpenOptions& {
  mode_ = mode;
  return *this;
}

fs::OpenOptions::OpenOptions()
    : read_(false),
      write_(false),
      append_(false),
      truncate_(false),
      create_(false),
      create_new_(false),
      custom_flags_(0),
      mode_(default_mode_) {}

auto fs::OpenOptions::get_access_mode() const -> io::IoResult<int> {
  if (read_ && !write_ && !append_) {
    return io::IoResult<int>::ok(O_RDONLY);
  }
  if (!read_ && write_ && !append_) {
    return io::IoResult<int>::ok(O_WRONLY);
  }
  if (read_ && write_ && !append_) {
    return io::IoResult<int>::ok(O_RDWR);
  }
  if (!read_ && append_) {
    return io::IoResult<int>::ok(O_WRONLY | O_APPEND);
  }
  if (read_ && append_) {
    return io::IoResult<int>::ok(O_RDWR | O_APPEND);
  }
  return io::IoResult<int>::err(io::IoError{.errno_ = EINVAL});
}

auto fs::OpenOptions::get_creation_mode() const -> io::IoResult<int> {
  if (write_ && !append_) {
    if (truncate_ || create_ || create_new_) {
      return io::IoResult<int>::err(io::IoError{.errno_ = EINVAL});
    }
  }
  if (append_) {
    if (truncate_ || create_new_) {
      return io::IoResult<int>::err(io::IoError{.errno_ = EINVAL});
    }
  }

  if (!create_ && !truncate_ && !create_new_) {
    return io::IoResult<int>::ok(0);
  }
  if (create_ && !truncate_ && !create_new_) {
    return io::IoResult<int>::ok(O_CREAT);
  }
  if (!create_ && truncate_ && !create_new_) {
    return io::IoResult<int>::ok(O_TRUNC);
  }
  if (create_ && truncate_ && !create_new_) {
    return io::IoResult<int>::ok(O_CREAT | O_TRUNC);
  }
  return io::IoResult<int>::ok(O_CREAT | O_EXCL);
}