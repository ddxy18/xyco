#include "xyco/fs/utils.h"

#include <expected>

#include "xyco/task/blocking_task.h"

auto xyco::fs::rename(std::filesystem::path old_path,
                      std::filesystem::path new_path)
    -> runtime::Future<utils::Result<void>> {
  co_return co_await task::BlockingTask([&]() {
    std::error_code error_code;
    std::filesystem::rename(old_path, new_path, error_code);

    return !error_code
               ? utils::Result<void>()
               : std::unexpected(utils::Error{.errno_ = error_code.value(),
                                              .info_ = error_code.message()});
  });
}

auto xyco::fs::remove(std::filesystem::path path)
    -> runtime::Future<utils::Result<bool>> {
  co_return co_await task::BlockingTask([&]() {
    std::error_code error_code;
    auto exist = std::filesystem::remove(path, error_code);

    return !error_code
               ? utils::Result<bool>(exist)
               : std::unexpected(utils::Error{.errno_ = error_code.value(),
                                              .info_ = error_code.message()});
  });
}

auto xyco::fs::copy_file(std::filesystem::path from_path,
                         std::filesystem::path to_path,
                         std::filesystem::copy_options options)
    -> runtime::Future<utils::Result<bool>> {
  co_return co_await task::BlockingTask([&]() {
    std::error_code error_code;
    auto ret =
        std::filesystem::copy_file(from_path, to_path, options, error_code);

    return !error_code
               ? utils::Result<bool>(ret)
               : std::unexpected(utils::Error{.errno_ = error_code.value(),
                                              .info_ = error_code.message()});
  });
}
