#ifndef XYCO_FS_UTILS_H_
#define XYCO_FS_UTILS_H_

#include <filesystem>

#include "runtime/future.h"
#include "utils/error.h"

namespace xyco::fs {
auto rename(const std::filesystem::path& old_path,
            const std::filesystem::path& new_path)
    -> runtime::Future<utils::Result<void>>;

auto remove(const std::filesystem::path& path)
    -> runtime::Future<utils::Result<bool>>;

auto copy_file(const std::filesystem::path& from_path,
               const std::filesystem::path& to_path,
               std::filesystem::copy_options options)
    -> runtime::Future<utils::Result<bool>>;
}  // namespace xyco::fs

#endif  // XYCO_FS_UTILS_H_