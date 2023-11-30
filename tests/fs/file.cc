#include <gtest/gtest.h>

#include <coroutine>
#include <filesystem>

import xyco.test.utils;
import xyco.fs;

class FileTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { std::filesystem::create_directory(fs_root_); }

  static void TearDownTestSuite() { std::filesystem::remove_all(fs_root_); }

  static const char *fs_root_;

  static constexpr auto OWNER_READ_FLAG = 0400;
};

const char *FileTest::fs_root_ = "test_root/";

TEST_F(FileTest, open_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_open_file";

    auto file_path = (std::string(fs_root_).append(path));
    *co_await xyco::fs::File::create(file_path);

    auto open_result = co_await xyco::fs::File::open(file_path);

    CO_ASSERT_EQ(open_result.has_value(), true);
  }());
}

TEST_F(FileTest, open_nonexist_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_open_nonexist_file";

    auto file_path = (std::string(fs_root_).append(path));

    auto open_result = co_await xyco::fs::File::open(file_path);

    CO_ASSERT_EQ(open_result.has_value(), false);
  }());
}

TEST_F(FileTest, all_mode_disable) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_all_mode_disable";

    auto file_path = (std::string(fs_root_).append(path));
    *co_await xyco::fs::File::create(file_path);

    auto open_result = co_await xyco::fs::OpenOptions().open(file_path);

    CO_ASSERT_EQ(open_result.has_value(), false);
  }());
}

TEST_F(FileTest, create_enable_write_disable_mode) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_create_enable_write_disable_mode";

    auto file_path = (std::string(fs_root_).append(path));
    auto open_result =
        co_await xyco::fs::OpenOptions().read(true).create(true).open(
            file_path);

    CO_ASSERT_EQ(open_result.has_value(), false);
  }());
}

TEST_F(FileTest, append_enable_truncate_enable_mode) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_append_enable_truncate_enable_mode";

    auto file_path = (std::string(fs_root_).append(path));
    auto open_result =
        co_await xyco::fs::OpenOptions().append(true).truncate(true).open(
            file_path);

    CO_ASSERT_EQ(open_result.has_value(), false);
  }());
}

TEST_F(FileTest, create_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_create_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto create_result = co_await xyco::fs::File::create(file_path);

    CO_ASSERT_EQ(create_result.has_value(), true);

    std::string content = "abc";
    *co_await create_result->write(content.begin(), content.end());
    auto open_result =
        co_await xyco::fs::OpenOptions().write(true).create(true).open(
            file_path);

    auto size = *co_await open_result->size();
    CO_ASSERT_EQ(size, content.size());
  }());
}

TEST_F(FileTest, create_new_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_create_new_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto open_result =
        co_await xyco::fs::OpenOptions().write(true).create_new(true).open(
            file_path);

    CO_ASSERT_EQ(open_result.has_value(), true);
  }());
}

TEST_F(FileTest, truncate_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_truncate_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto create_result = co_await xyco::fs::File::create(file_path);
    std::string content = "abc";
    *co_await create_result->write(content.begin(), content.end());
    auto truncate_result =
        co_await xyco::fs::OpenOptions().write(true).truncate(true).open(
            file_path);

    auto size = *co_await truncate_result->size();
    CO_ASSERT_EQ(size, 0);
  }());
}

TEST_F(FileTest, append_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_append_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto create_result = co_await xyco::fs::File::create(file_path);
    std::string content = "abc";
    *co_await create_result->write(content.begin(), content.end());
    auto append_result =
        co_await xyco::fs::OpenOptions().read(true).append(true).open(
            file_path);
    *co_await append_result->write(content.begin(), content.end());

    auto size = *co_await append_result->size();
    CO_ASSERT_EQ(size, 2 * content.size());
  }());
}

TEST_F(FileTest, file_permisssion) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_permission_file";

    auto file_path = (std::string(fs_root_).append(path));
    *co_await xyco::fs::OpenOptions()
         .create(true)
         .write(true)
         .truncate(true)
         .mode(OWNER_READ_FLAG)
         .open(file_path);
    auto open_result =
        co_await xyco::fs::OpenOptions().write(true).open(file_path);

    CO_ASSERT_EQ(open_result.error().errno_, EACCES);
  }());
}

TEST_F(FileTest, resize_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_resize_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = *co_await xyco::fs::File::create(file_path);

    auto resize_result = co_await file.resize(4);

    CO_ASSERT_EQ(resize_result.has_value(), true);

    auto size = *co_await file.size();

    CO_ASSERT_EQ(size, 4);
  }());
}

TEST_F(FileTest, file_status) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_file_status";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = *co_await xyco::fs::OpenOptions()
                     .create(true)
                     .write(true)
                     .mode(OWNER_READ_FLAG)
                     .open(file_path);

    auto status = *co_await file.status();

    CO_ASSERT_EQ(status.type(), std::filesystem::file_type::regular);
    CO_ASSERT_EQ(status.permissions(), std::filesystem::perms::owner_read);
  }());
}

TEST_F(FileTest, file_attr) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_file_attr";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = *co_await xyco::fs::File::create(file_path);

    auto create_time = *co_await file.created();
    auto modified_time = *co_await file.modified();
    auto access_time = *co_await file.accessed();

    CO_ASSERT_EQ(create_time.tv_sec, modified_time.tv_sec);
    CO_ASSERT_EQ(create_time.tv_sec, access_time.tv_sec);
  }());
}

TEST_F(FileTest, set_file_permission) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_set_file_permission";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = *co_await xyco::fs::File::create(file_path);

    *co_await file.set_permissions(std::filesystem::perms::group_write,
                                   std::filesystem::perm_options::add);

    auto permissions = (co_await file.status())->permissions();

    CO_ASSERT_EQ(permissions, std::filesystem::perms::others_read |
                                  std::filesystem::perms::group_read |
                                  std::filesystem::perms::group_write |
                                  std::filesystem::perms::owner_read |
                                  std::filesystem::perms::owner_write);
  }());
}

TEST_F(FileTest, rename_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_rename_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = *co_await xyco::fs::File::create(file_path);

    auto new_file_path = std::string(file_path).append("1");
    *co_await (xyco::fs::rename(file_path, new_file_path));

    auto open_new_file_result = (co_await xyco::fs::File::open(new_file_path));
    CO_ASSERT_EQ(open_new_file_result.has_value(), true);
  }());
}

TEST_F(FileTest, rename_nonexist_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_rename_nonexist_file";

    auto file_path = std::string(fs_root_).append(path);

    auto new_file_path = std::string(file_path).append("1");
    auto rename_result = co_await (xyco::fs::rename(file_path, new_file_path));

    CO_ASSERT_EQ(rename_result.has_value(), false);
  }());
}

TEST_F(FileTest, remove_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_remove_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = *co_await xyco::fs::File::create(file_path);

    *co_await (xyco::fs::remove(file_path));

    auto open_result = (co_await xyco::fs::File::open(file_path));
    CO_ASSERT_EQ(open_result.has_value(), false);
  }());
}

TEST_F(FileTest, remove_file_permission_denied) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    auto remove_result = co_await (xyco::fs::remove("/proc/1"));

    CO_ASSERT_EQ(remove_result.has_value(), false);
  }());
}

TEST_F(FileTest, copy_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_copy_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = *co_await xyco::fs::File::create(file_path);
    *co_await file.resize(4);

    auto new_file_path = std::string(file_path).append("_copy");
    *co_await (
        xyco::fs::copy_file(file_path, new_file_path,
                            std::filesystem::copy_options::overwrite_existing));

    auto new_file_size =
        *co_await (co_await xyco::fs::File::open(new_file_path))->size();

    CO_ASSERT_EQ(new_file_size, 4);
  }());
}

TEST_F(FileTest, copy_nonexist_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_copy_nonexist_file";

    auto file_path = std::string(fs_root_).append(path);

    auto new_file_path = std::string(file_path).append("_copy");
    auto copy_result = co_await (
        xyco::fs::copy_file(file_path, new_file_path,
                            std::filesystem::copy_options::overwrite_existing));

    CO_ASSERT_EQ(copy_result.has_value(), false);
  }());
}

TEST_F(FileTest, operate_removed_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_operate_removed_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = *co_await xyco::fs::File::create(file_path);

    *co_await (xyco::fs::remove(file_path));

    auto resize_result = co_await file.resize(4);
    CO_ASSERT_EQ(resize_result.has_value(), false);

    auto size_result = (co_await file.size());
    CO_ASSERT_EQ(size_result.has_value(), false);

    auto status_result = co_await file.status();
    CO_ASSERT_EQ(status_result.has_value(), false);

    auto set_permissions_result =
        co_await file.set_permissions(std::filesystem::perms::group_write,
                                      std::filesystem::perm_options::add);
    CO_ASSERT_EQ(set_permissions_result.has_value(), false);
  }());
}

TEST_F(FileTest, rw_file) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_rw_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = *co_await xyco::fs::OpenOptions()
                     .read(true)
                     .write(true)
                     .create_new(true)
                     .open(file_path);

    auto write_content = std::string("abcd");
    auto write_result =
        (co_await file.write(write_content.begin(), write_content.end()));

    CO_ASSERT_EQ(*write_result, write_content.size());

    *co_await file.seek(0, SEEK_DATA);
    auto read_content = std::string(write_content.size(), 0);
    auto read_result =
        (co_await file.read(read_content.begin(), read_content.end()));

    CO_ASSERT_EQ(read_content, write_content);
  }());
}

TEST_F(FileTest, seek_invalid) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_seek_invalid";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = *co_await xyco::fs::File::create(file_path);

    auto write_content = std::string("abcd");
    auto write_result =
        (co_await file.write(write_content.begin(), write_content.end()));

    auto seek_result = (co_await file.seek(4, SEEK_DATA));

    CO_ASSERT_EQ(seek_result.has_value(), false);
  }());
}

TEST_F(FileTest, flush) {
  TestRuntimeCtx::runtime()->block_on([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_flush";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = *co_await xyco::fs::File::create(file_path);

    auto write_content = std::string("abcd");
    *co_await file.write(write_content.begin(), write_content.end());
    auto flush_result = (co_await file.flush());

    CO_ASSERT_EQ(flush_result.has_value(), true);
  }());
}
