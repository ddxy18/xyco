module;

#include <sys/sysmacros.h>

#include <coroutine>
#include <expected>
#include <filesystem>
#include <utility>

#include "xyco/utils/result.h"

module xyco.fs.epoll;

import xyco.task;
import xyco.libc;

auto get_file_attr(int file_descriptor)
    -> xyco::runtime::Future<xyco::utils::Result<
        std::pair<xyco::libc::stat64_t, xyco::fs::StatxExtraFields>>> {
  xyco::libc::statx_t stx{};
  xyco::libc::stat64_t stat{};

  ASYNC_TRY((co_await xyco::task::BlockingTask([&]() {
              return xyco::utils::into_sys_result(
                  xyco::libc::statx(file_descriptor, "\0",
                                    xyco::libc::K_AT_EMPTY_PATH |
                                        xyco::libc::K_AT_STATX_SYNC_AS_STAT,
                                    xyco::libc::K_STATX_ALL, &stx));
            })).transform([]([[maybe_unused]] auto n) {
    return std::pair<xyco::libc::stat64_t, xyco::fs::StatxExtraFields>({}, {});
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

  co_return std::pair<xyco::libc::stat64_t, xyco::fs::StatxExtraFields>(
      stat, {
                .stx_mask_ = stx.stx_mask,
                .stx_btime_ = stx.stx_btime,
            });
}

auto xyco::fs::epoll::File::modified() const
    -> runtime::Future<utils::Result<timespec>> {
  co_return (co_await get_file_attr(fd_)).transform([](auto pair) {
    auto stat = pair.first;
    return timespec{.tv_sec = stat.st_mtim.tv_sec,
                    .tv_nsec = stat.st_mtim.tv_nsec};
  });
}

auto xyco::fs::epoll::File::accessed() const
    -> runtime::Future<utils::Result<timespec>> {
  co_return (co_await get_file_attr(fd_)).transform([](auto pair) {
    auto stat = pair.first;
    return timespec{.tv_sec = stat.st_atim.tv_sec,
                    .tv_nsec = stat.st_atim.tv_nsec};
  });
}

auto xyco::fs::epoll::File::created() const
    -> runtime::Future<utils::Result<timespec>> {
  auto result = co_await get_file_attr(fd_);
  if (result) {
    auto ext = result->second;
    if ((ext.stx_mask_ & xyco::libc::K_STATX_BTIME) != 0) {
      co_return timespec{.tv_sec = ext.stx_btime_.tv_sec,
                         .tv_nsec = ext.stx_btime_.tv_nsec};
    }
    co_return std::unexpected(utils::Error{
        .errno_ = std::to_underlying(utils::ErrorKind::Uncategorized),
        .info_ = "creation time is not available for the filesystem"});
  }
  co_return std::unexpected(utils::Error{
      .errno_ = std::to_underlying(utils::ErrorKind::Unsupported),
      .info_ = "creation time is not available on this platform currently"});
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

auto xyco::fs::epoll::File::set_permissions(std::filesystem::perms prms,
                                            std::filesystem::perm_options opts)
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
    return utils::into_sys_result(xyco::libc::fsync(fd_))
        .transform([]([[maybe_unused]] auto result) {});
  });
}

xyco::fs::epoll::File::File(int file_descriptor, std::filesystem::path &&path)
    : FileBase(file_descriptor, std::move(path)) {}

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
    int flags = xyco::libc::K_O_CLOEXEC | *access_mode | *creation_mode;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return utils::into_sys_result(xyco::libc::open(path.c_str(), flags, mode_))
        .transform([&](auto file_descriptor) {
          return File(file_descriptor, std::move(path));
        });
  });
}
