#include "utils.h"

#include "io/utils.h"
#include "runtime/async_future.h"

auto xyco::fs::rename(const std::filesystem::path& old_path,
                      const std::filesystem::path& new_path)
    -> runtime::Future<io::IoResult<void>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<void>>([&]() {
    std::error_code error_code;
    std::filesystem::rename(old_path, new_path, error_code);

    return !error_code ? io::IoResult<void>::ok()
                       : io::IoResult<void>::err(
                             io::IoError{.errno_ = error_code.value(),
                                         .info_ = error_code.message()});
  });
}

auto xyco::fs::remove(const std::filesystem::path& path)
    -> runtime::Future<io::IoResult<bool>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<bool>>([&]() {
    std::error_code error_code;
    auto exist = std::filesystem::remove(path, error_code);

    return !error_code ? io::IoResult<bool>::ok(exist)
                       : io::IoResult<bool>::err(
                             io::IoError{.errno_ = error_code.value(),
                                         .info_ = error_code.message()});
  });
}

auto xyco::fs::copy_file(const std::filesystem::path& from_path,
                         const std::filesystem::path& to_path,
                         std::filesystem::copy_options options)
    -> runtime::Future<io::IoResult<bool>> {
  co_return co_await runtime::AsyncFuture<io::IoResult<bool>>([&]() {
    std::error_code error_code;
    auto ret =
        std::filesystem::copy_file(from_path, to_path, options, error_code);

    return !error_code ? io::IoResult<bool>::ok(ret)
                       : io::IoResult<bool>::err(
                             io::IoError{.errno_ = error_code.value(),
                                         .info_ = error_code.message()});
  });
}