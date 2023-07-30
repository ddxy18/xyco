#ifndef XYCO_FS_IO_URING_FILE_H_
#define XYCO_FS_IO_URING_FILE_H_

#include <fcntl.h>
#include <linux/stat.h>
#include <unistd.h>

#include <filesystem>

#include "io/io_uring/extra.h"
#include "io/io_uring/registry.h"
#include "runtime/async_future.h"
#include "runtime/runtime_ctx.h"
#include "utils/error.h"
#include "utils/logger.h"

namespace xyco::fs::uring {
class File {
  friend class OpenOptions;
  friend struct std::formatter<File>;

 public:
  static auto create(std::filesystem::path path)
      -> runtime::Future<utils::Result<File>>;

  static auto open(std::filesystem::path path)
      -> runtime::Future<utils::Result<File>>;

  auto resize(uintmax_t size) -> runtime::Future<utils::Result<void>>;

  [[nodiscard]] auto size() const -> runtime::Future<utils::Result<uintmax_t>>;

  auto status() -> runtime::Future<utils::Result<std::filesystem::file_status>>;

  [[nodiscard]] auto set_permissions(std::filesystem::perms prms,
                                     std::filesystem::perm_options opts =
                                         std::filesystem::perm_options::replace)
      const -> runtime::Future<utils::Result<void>>;

  [[nodiscard]] auto modified() const
      -> runtime::Future<utils::Result<timespec>>;

  [[nodiscard]] auto accessed() const
      -> runtime::Future<utils::Result<timespec>>;

  [[nodiscard]] auto created() const
      -> runtime::Future<utils::Result<timespec>>;

  template <typename Iterator>
  auto read(Iterator begin, Iterator end)
      -> runtime::Future<utils::Result<uintptr_t>> {
    using CoOutput = utils::Result<uintptr_t>;

    class Future : public runtime::Future<CoOutput> {
     public:
      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<CoOutput> override {
        auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());

        if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
          event_->future_ = this;
          extra->args_ = io::uring::IoExtra::Read{
              .buf_ = &*begin_,
              .len_ = static_cast<unsigned int>(std::distance(begin_, end_))};
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .Register<io::uring::IoRegistry>(event_);
          return runtime::Pending();
        }
        extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
        if (extra->return_ >= 0) {
          INFO("read {} bytes from {}", extra->return_, self_->fd_);
          return runtime::Ready<CoOutput>{CoOutput::ok(extra->return_)};
        }
        return runtime::Ready<CoOutput>{
            CoOutput::err(utils::Error{.errno_ = -extra->return_})};
      }

      Future(Iterator begin, Iterator end, File *self)
          : runtime::Future<CoOutput>(nullptr),
            event_(std::make_shared<runtime::Event>(runtime::Event{
                .extra_ = std::make_unique<io::uring::IoExtra>()})),
            begin_(begin),
            end_(end),
            self_(self) {
        auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
        extra->fd_ = self_->fd_;
      }

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(const Future &future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(Future &&future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(Future &&future) -> Future & = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(const Future &future) -> Future & = delete;

      ~Future() override = default;

     private:
      File *self_;
      std::shared_ptr<runtime::Event> event_;
      Iterator begin_;
      Iterator end_;
    };

    co_return co_await Future(begin, end, this);
  }

  template <typename Iterator>
  auto write(Iterator begin, Iterator end)
      -> runtime::Future<utils::Result<uintptr_t>> {
    using CoOutput = utils::Result<uintptr_t>;

    class Future : public runtime::Future<CoOutput> {
     public:
      auto poll(runtime::Handle<void> self)
          -> runtime::Poll<CoOutput> override {
        auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
        if (!extra->state_.get_field<io::uring::IoExtra::State::Completed>()) {
          event_->future_ = this;
          extra->args_ = io::uring::IoExtra::Write{
              .buf_ = &*begin_,
              .len_ = static_cast<unsigned int>(std::distance(begin_, end_))};
          runtime::RuntimeCtx::get_ctx()
              ->driver()
              .Register<io::uring::IoRegistry>(event_);

          return runtime::Pending();
        }
        extra->state_.set_field<io::uring::IoExtra::State::Completed, false>();
        if (extra->return_ >= 0) {
          INFO("write {} bytes to {}", extra->return_, self_->fd_);
          return runtime::Ready<CoOutput>{CoOutput::ok(extra->return_)};
        }
        return runtime::Ready<CoOutput>{
            CoOutput::err(utils::Error{.errno_ = -extra->return_})};
      }

      Future(Iterator begin, Iterator end, File *self)
          : runtime::Future<CoOutput>(nullptr),
            event_(std::make_shared<runtime::Event>(runtime::Event{
                .extra_ = std::make_unique<io::uring::IoExtra>()})),
            begin_(begin),
            end_(end),
            self_(self) {
        auto *extra = dynamic_cast<io::uring::IoExtra *>(event_->extra_.get());
        extra->fd_ = self_->fd_;
      }

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(const Future &future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      Future(Future &&future) = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(Future &&future) -> Future & = delete;

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-reference-coroutine-parameters)
      auto operator=(const Future &future) -> Future & = delete;

      ~Future() override = default;

     private:
      File *self_;
      std::shared_ptr<runtime::Event> event_;
      Iterator begin_;
      Iterator end_;
    };

    co_return co_await Future(begin, end, this);
  }

  [[nodiscard]] auto flush() const -> runtime::Future<utils::Result<void>>;

  [[nodiscard]] auto seek(off64_t offset, int whence) const
      -> runtime::Future<utils::Result<off64_t>>;

  File(const File &file) = delete;

  File(File &&file) noexcept;

  auto operator=(const File &file) = delete;

  auto operator=(File &&file) noexcept -> File &;

  ~File();

 private:
  File(int file_descriptor, std::filesystem::path &&path);

  int fd_;
  std::filesystem::path path_;
};

class OpenOptions {
 public:
  auto open(std::filesystem::path path) -> runtime::Future<utils::Result<File>>;

  auto read(bool read) -> OpenOptions &;

  auto write(bool write) -> OpenOptions &;

  auto truncate(bool truncate) -> OpenOptions &;

  auto append(bool append) -> OpenOptions &;

  auto create(bool create) -> OpenOptions &;

  auto create_new(bool create_new) -> OpenOptions &;

  auto mode(uint32_t mode) -> OpenOptions &;

  OpenOptions();

 private:
  [[nodiscard]] auto get_access_mode() const -> utils::Result<int>;

  [[nodiscard]] auto get_creation_mode() const -> utils::Result<int>;

  static constexpr int default_mode_ = 0666;

  // generic
  bool read_;
  bool write_;
  bool append_;
  bool truncate_;
  bool create_;
  bool create_new_;
  // system-specific
  mode_t mode_;
};
}  // namespace xyco::fs::uring

template <>
struct std::formatter<xyco::fs::uring::File> : public std::formatter<bool> {
  template <typename FormatContext>
  auto format(const xyco::fs::uring::File &file, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "File{{path_={}}}", file.path_.c_str());
  }
};

#endif  // XYCO_FS_IO_URING_FILE_H_
