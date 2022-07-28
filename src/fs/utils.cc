#include "utils.h"

#include "runtime/async_future.h"

auto xyco::fs::rename(const std::filesystem::path& old_path,
                      const std::filesystem::path& new_path)
    -> runtime::Future<utils::Result<void>> {
  co_return co_await runtime::AsyncFuture<utils::Result<void>>([&]() {
    std::error_code error_code;
    std::filesystem::rename(old_path, new_path, error_code);

    return !error_code ? utils::Result<void>::ok()
                       : utils::Result<void>::err(
                             utils::Error{.errno_ = error_code.value(),
                                          .info_ = error_code.message()});
  });
}

auto xyco::fs::remove(const std::filesystem::path& path)
    -> runtime::Future<utils::Result<bool>> {
  co_return co_await runtime::AsyncFuture<utils::Result<bool>>([&]() {
    std::error_code error_code;
    auto exist = std::filesystem::remove(path, error_code);

    return !error_code ? utils::Result<bool>::ok(exist)
                       : utils::Result<bool>::err(
                             utils::Error{.errno_ = error_code.value(),
                                          .info_ = error_code.message()});
  });
}

auto xyco::fs::copy_file(const std::filesystem::path& from_path,
                         const std::filesystem::path& to_path,
                         std::filesystem::copy_options options)
    -> runtime::Future<utils::Result<bool>> {
  co_return co_await runtime::AsyncFuture<utils::Result<bool>>([&]() {
    std::error_code error_code;
    auto ret =
        std::filesystem::copy_file(from_path, to_path, options, error_code);

    return !error_code ? utils::Result<bool>::ok(ret)
                       : utils::Result<bool>::err(
                             utils::Error{.errno_ = error_code.value(),
                                          .info_ = error_code.message()});
  });
}