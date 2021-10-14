
#include "file.h"

#include <__utility/to_underlying.h>
#include <sys/stat.h>
#include <unistd.h>

#include "runtime/async_future.h"
#include "utils/result.h"

auto fs::Permissions::set_readonly(bool readonly) -> void {
  const int readonly_mode = 0222;
  if (readonly) {
    // remove write permission for all classes; equivalent to `chmod a-w
    // <file>`
    mode_ &= ~readonly_mode;
  } else {
    // add write permission for all classes; equivalent to `chmod a+w <file>`
    mode_ |= readonly_mode;
  }
}

fs::Permissions::Permissions(mode_t mode) : mode_(mode) {}

auto fs::FileType::is_dir() const -> bool { return is(S_IFDIR); }

auto fs::FileType::is_file() const -> bool { return is(S_IFREG); }

auto fs::FileType::is_symlink() const -> bool { return is(S_IFLNK); }

auto fs::FileType::is(mode_t mode) const -> bool {
  return (mode_ & S_IFMT) == mode;
}

fs::FileType::FileType(mode_t mode) : mode_(mode) {}

auto fs::FileAttr::file_type() const -> FileType { return {stat_.st_mode}; }

auto fs::FileAttr::is_dir() const -> bool { return file_type().is_dir(); }

auto fs::FileAttr::is_file() const -> bool { return file_type().is_file(); }

auto fs::FileAttr::is_symlink() const -> bool {
  return file_type().is_symlink();
}

auto fs::FileAttr::len() const -> uint64_t { return stat_.st_size; }

auto fs::FileAttr::permissions() const -> Permissions {
  return {stat_.st_mode};
}

auto fs::FileAttr::modified() -> io::IoResult<timespec> {
  return io::IoResult<timespec>::ok(timespec{.tv_sec = stat_.st_mtim.tv_sec,
                                             .tv_nsec = stat_.st_mtim.tv_nsec});
}

auto fs::FileAttr::accessed() -> io::IoResult<timespec> {
  return io::IoResult<timespec>::ok(timespec{.tv_sec = stat_.st_atim.tv_sec,
                                             .tv_nsec = stat_.st_atim.tv_nsec});
}

auto fs::FileAttr::created() -> io::IoResult<timespec> {
  if (statx_extra_fields_.has_value()) {
    auto ext = statx_extra_fields_.value();
    if ((ext.stx_mask_ & STATX_BTIME) != 0) {
      return io::IoResult<timespec>::ok(timespec{
          .tv_sec = ext.stx_btime_.tv_sec, .tv_nsec = ext.stx_btime_.tv_nsec});
    }
    return io::IoResult<timespec>::err(io::IoError{
        .errno_ = std::__to_underlying(io::ErrorKind::Uncategorized),
        .info_ = "creation time is not available for the filesystem"});
  }
  return io::IoResult<timespec>::err(
      io::IoError{.errno_ = std::__to_underlying(io::ErrorKind::Unsupported),
                  .info_ = "creation time is not available on this platform \
                            currently"});
}

auto fs::File::create(std::filesystem::path path)
    -> runtime::Future<io::IoResult<File>> {
  co_return co_await OpenOptions().write(true).create(true).truncate(true).open(
      path);
}

auto fs::File::open(std::filesystem::path path)
    -> runtime::Future<io::IoResult<File>> {
  co_return co_await OpenOptions().read(true).open(path);
}

auto fs::File::set_permissions(Permissions permissions) const
    -> runtime::Future<io::IoResult<void>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<void>>(
      [&]() { return io::into_sys_result(::fchmod(fd_, permissions.mode_)); });
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

auto fs::OpenOptions::open(std::filesystem::path path)
    -> runtime::Future<io::IoResult<File>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<File>>([&]() {
    auto access_mode = get_access_mode();
    if (access_mode.is_err()) {
      return access_mode.map([](auto n) { return File(-1); });
    }
    auto creation_mode = get_creation_mode();
    if (creation_mode.is_err()) {
      return creation_mode.map([](auto n) { return File(-1); });
    }

    int flags = O_CLOEXEC | access_mode.unwrap() | creation_mode.unwrap() |
                (custom_flags_ & O_ACCMODE);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return io::into_sys_result(::open(path.c_str(), flags, mode_))
        .map([](auto fd) { return File(fd); });
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