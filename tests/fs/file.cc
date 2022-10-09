#include <gtest/gtest.h>

#include "fs/utils.h"
#include "utils.h"

class FileTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { std::filesystem::create_directory(fs_root_); }

  static void TearDownTestSuite() { std::filesystem::remove_all(fs_root_); }

  static const char *fs_root_;
};

const char *FileTest::fs_root_ = "test_root/";

TEST_F(FileTest, open_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_open_file";

    auto file_path = (std::string(fs_root_).append(path));
    (co_await xyco::fs::File::create(file_path)).unwrap();

    auto open_result = co_await xyco::fs::File::open(file_path);

    CO_ASSERT_EQ(open_result.is_ok(), true);
  });
}

TEST_F(FileTest, open_nonexist_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_open_nonexist_file";

    auto file_path = (std::string(fs_root_).append(path));

    auto open_result = co_await xyco::fs::File::open(file_path);

    CO_ASSERT_EQ(open_result.is_err(), true);
  });
}

TEST_F(FileTest, all_mode_disable) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_all_mode_disable";

    auto file_path = (std::string(fs_root_).append(path));
    (co_await xyco::fs::File::create(file_path)).unwrap();

    auto open_result = co_await xyco::fs::OpenOptions().open(file_path);

    CO_ASSERT_EQ(open_result.is_err(), true);
  });
}

TEST_F(FileTest, create_enable_write_disable_mode) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_create_enable_write_disable_mode";

    auto file_path = (std::string(fs_root_).append(path));
    auto open_result =
        co_await xyco::fs::OpenOptions().read(true).create(true).open(
            file_path);

    CO_ASSERT_EQ(open_result.is_err(), true);
  });
}

TEST_F(FileTest, append_enable_truncate_enable_mode) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_append_enable_truncate_enable_mode";

    auto file_path = (std::string(fs_root_).append(path));
    auto open_result =
        co_await xyco::fs::OpenOptions().append(true).truncate(true).open(
            file_path);

    CO_ASSERT_EQ(open_result.is_err(), true);
  });
}

TEST_F(FileTest, create_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_create_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto create_result = co_await xyco::fs::File::create(file_path);

    CO_ASSERT_EQ(create_result.is_ok(), true);
  });
}

TEST_F(FileTest, create_new_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_create_new_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto open_result =
        co_await xyco::fs::OpenOptions().write(true).create_new(true).open(
            file_path);

    CO_ASSERT_EQ(open_result.is_ok(), true);
  });
}

TEST_F(FileTest, resize_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_resize_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    auto resize_result = co_await file.resize(4);

    CO_ASSERT_EQ(resize_result.is_ok(), true);

    auto size = (co_await file.size()).unwrap();

    CO_ASSERT_EQ(size, 4);
  });
}

TEST_F(FileTest, file_status) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_file_status";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    auto status = (co_await file.status()).unwrap();

    CO_ASSERT_EQ(status.type(), std::filesystem::file_type::regular);
    CO_ASSERT_EQ(status.permissions(), std::filesystem::perms::others_read |
                                           std::filesystem::perms::group_read |
                                           std::filesystem::perms::owner_read |
                                           std::filesystem::perms::owner_write);
  });
}

TEST_F(FileTest, file_attr) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_file_attr";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    auto create_time = (co_await file.created()).unwrap();
    auto modified_time = (co_await file.modified()).unwrap();
    auto access_time = (co_await file.accessed()).unwrap();

    CO_ASSERT_EQ(create_time.tv_sec, modified_time.tv_sec);
    CO_ASSERT_EQ(create_time.tv_sec, access_time.tv_sec);
  });
}

TEST_F(FileTest, set_file_permission) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_set_file_permission";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    (co_await file.set_permissions(std::filesystem::perms::group_write,
                                   std::filesystem::perm_options::add))
        .unwrap();

    auto permissions = (co_await file.status()).unwrap().permissions();

    CO_ASSERT_EQ(permissions, std::filesystem::perms::others_read |
                                  std::filesystem::perms::group_read |
                                  std::filesystem::perms::group_write |
                                  std::filesystem::perms::owner_read |
                                  std::filesystem::perms::owner_write);
  });
}

TEST_F(FileTest, rename_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_rename_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    auto new_file_path = std::string(file_path).append("1");
    (co_await (xyco::fs::rename(file_path, new_file_path))).unwrap();

    auto open_new_file_result = (co_await xyco::fs::File::open(new_file_path));
    CO_ASSERT_EQ(open_new_file_result.is_ok(), true);
  });
}

TEST_F(FileTest, rename_nonexist_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_rename_nonexist_file";

    auto file_path = std::string(fs_root_).append(path);

    auto new_file_path = std::string(file_path).append("1");
    auto rename_result = co_await (xyco::fs::rename(file_path, new_file_path));

    CO_ASSERT_EQ(rename_result.is_err(), true);
  });
}

TEST_F(FileTest, remove_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_remove_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    (co_await (xyco::fs::remove(file_path))).unwrap();

    auto open_result = (co_await xyco::fs::File::open(file_path));
    CO_ASSERT_EQ(open_result.is_ok(), false);
  });
}

TEST_F(FileTest, remove_file_permission_denied) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    auto remove_result = co_await (xyco::fs::remove("/proc/1"));

    CO_ASSERT_EQ(remove_result.is_ok(), false);
  });
}

TEST_F(FileTest, copy_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_copy_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();
    (co_await file.resize(4)).unwrap();

    auto new_file_path = std::string(file_path).append("_copy");
    (co_await (xyco::fs::copy_file(
         file_path, new_file_path,
         std::filesystem::copy_options::overwrite_existing)))
        .unwrap();

    auto new_file_size =
        (co_await (co_await xyco::fs::File::open(new_file_path))
             .unwrap()
             .size())
            .unwrap();

    CO_ASSERT_EQ(new_file_size, 4);
  });
}

TEST_F(FileTest, copy_nonexist_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_copy_nonexist_file";

    auto file_path = std::string(fs_root_).append(path);

    auto new_file_path = std::string(file_path).append("_copy");
    auto copy_result = co_await (
        xyco::fs::copy_file(file_path, new_file_path,
                            std::filesystem::copy_options::overwrite_existing));

    CO_ASSERT_EQ(copy_result.is_err(), true);
  });
}

TEST_F(FileTest, operate_removed_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_operate_removed_file";

    auto file_path = std::string(fs_root_).append(path);
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    (co_await (xyco::fs::remove(file_path))).unwrap();

    auto resize_result = co_await file.resize(4);
    CO_ASSERT_EQ(resize_result.is_err(), true);

    auto size_result = (co_await file.size());
    CO_ASSERT_EQ(size_result.is_err(), true);

    auto status_result = co_await file.status();
    CO_ASSERT_EQ(status_result.is_err(), true);

    auto set_permissions_result =
        co_await file.set_permissions(std::filesystem::perms::group_write,
                                      std::filesystem::perm_options::add);
    CO_ASSERT_EQ(set_permissions_result.is_err(), true);
  });
}

TEST_F(FileTest, rw_file) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_rw_file";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = (co_await xyco::fs::OpenOptions()
                     .read(true)
                     .write(true)
                     .create_new(true)
                     .open(file_path))
                    .unwrap();

    auto write_content = std::string("abcd");
    auto write_result =
        (co_await file.write(write_content.begin(), write_content.end()));

    CO_ASSERT_EQ(write_result.unwrap(), write_content.size());

    (co_await file.seek(0, SEEK_DATA)).unwrap();
    auto read_content = std::string(write_content.size(), 0);
    auto read_result =
        (co_await file.read(read_content.begin(), read_content.end()));

    CO_ASSERT_EQ(read_content, write_content);
  });
}

TEST_F(FileTest, seek_invalid) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_seek_invalid";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    auto write_content = std::string("abcd");
    auto write_result =
        (co_await file.write(write_content.begin(), write_content.end()));

    auto seek_result = (co_await file.seek(4, SEEK_DATA));

    CO_ASSERT_EQ(seek_result.is_err(), true);
  });
}

TEST_F(FileTest, flush) {
  TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
    const char *path = "test_flush";

    auto file_path = (std::string(fs_root_).append(path));
    auto file = (co_await xyco::fs::File::create(file_path)).unwrap();

    auto write_content = std::string("abcd");
    (co_await file.write(write_content.begin(), write_content.end())).unwrap();
    auto flush_result = (co_await file.flush());

    CO_ASSERT_EQ(flush_result.is_ok(), true);
  });
}