#include <gtest/gtest.h>

#include <gsl/pointers>

#include "runtime/runtime.h"
#include "utils.h"

class InRuntimeTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

class NoRuntimeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rt_ = nullptr;
    xyco::runtime::RuntimeCtx::set_ctx(rt_.get());
  }

 private:
  std::unique_ptr<xyco::runtime::Runtime> rt_;
};

TEST_F(InRuntimeTest, NoSuspend) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    int co_result = -1;
    auto co_innner = []() -> xyco::runtime::Future<int> { co_return 1; };
    co_result = co_await co_innner();

    CO_ASSERT_EQ(co_result, 1);
  });
}

TEST_F(InRuntimeTest, Suspend) {
  std::atomic_int co_result = -1;
  xyco::runtime::Future<int> *handle = nullptr;
  auto co_outer =
      TestRuntimeCtx::co_run_no_wait([&]() -> xyco::runtime::Future<void> {
        auto co_innner = [&]() -> xyco::runtime::Future<int> {
          class SuspendFuture : public xyco::runtime::Future<int> {
           public:
            SuspendFuture(xyco::runtime::Future<int> *&handle)
                : xyco::runtime::Future<int>(nullptr), handle_(handle) {}

            [[nodiscard]] auto poll(xyco::runtime::Handle<void> self)
                -> xyco::runtime::Poll<int> override {
              if (!ready_) {
                ready_ = true;
                handle_ = this;
                return xyco::runtime::Pending();
              }
              return xyco::runtime::Ready<int>{1};
            }

           private:
            bool ready_{false};
            xyco::runtime::Future<int> *&handle_;
          };
          co_return co_await SuspendFuture(handle);
        };
        co_result = co_await co_innner();
      });
  if (handle->poll_wrapper()) {
    handle->get_handle().resume();
  }

  ASSERT_EQ(co_result, 1);
}

TEST_F(InRuntimeTest, NoSuspend_loop) {
  constexpr int ITERATION_TIMES = 100000;

  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto co_innner = []() -> xyco::runtime::Future<int> { co_return 1; };

    for (int i = 0; i < ITERATION_TIMES; i++) {
      int co_result = -1;
      co_result = co_await co_innner();

      CO_ASSERT_EQ(co_result, 1);
    }
  });
}

TEST_F(NoRuntimeTest, NeverRun) {
  std::atomic_int co_result = -1;
  auto co_outer = TestRuntimeCtx::co_run_without_runtime(
      [&]() -> xyco::runtime::Future<void> {
        auto co_innner = []() -> xyco::runtime::Future<int> { co_return 1; };
        co_result = co_await co_innner();
      });

  ASSERT_EQ(co_result, -1);
}

TEST(RuntimeDeathTest, terminate) {
  EXPECT_DEATH(
      {
        // Default handler bypasses process finalization which skips coverage
        // data flushing. So use `exit` as terminate handler to produce correct
        // coverage statistics.
        std::set_terminate([] { std::exit(-1); });

        auto rt = xyco::runtime::Builder::new_multi_thread()
                      .worker_threads(1)
                      .max_blocking_threads(1)
                      .build()
                      .unwrap();
        rt->spawn([]() -> xyco::runtime::Future<void> {
          throw std::runtime_error("");
          co_return;
        }());
        std::this_thread::sleep_for(2 * time_deviation);
      },
      "");
}

TEST(RuntimeDeathTest, coroutine_exception) {
  EXPECT_EXIT(
      {
        auto rt = xyco::runtime::Builder::new_multi_thread()
                      .worker_threads(2)
                      .max_blocking_threads(1)
                      .build()
                      .unwrap();
        rt->spawn([]() -> xyco::runtime::Future<void> {
          throw std::runtime_error("");
          co_return;
        }());

        std::atomic_int result = -1;
        auto fut = [&]() -> xyco::runtime::Future<void> {
          result = 1;
          co_return;
        };
        rt->spawn(fut());
        std::this_thread::sleep_for(time_deviation);

        ASSERT_EQ(result, 1);

        std::exit(0);
      },
      testing::ExitedWithCode(0), "");
}

class DropAsserter {
 public:
  DropAsserter(char member) : member_(member) { dropped_ = false; }

  DropAsserter(const DropAsserter &drop_asserter) = delete;

  DropAsserter(DropAsserter &&drop_asserter) noexcept {
    *this = std::move(drop_asserter);
  }

  auto operator=(const DropAsserter &drop_asserter) -> DropAsserter & = delete;

  auto operator=(DropAsserter &&drop_asserter) noexcept -> DropAsserter & {
    member_ = drop_asserter.member_;
    drop_asserter.member_ = -1;

    return *this;
  }

  static auto assert_drop() { ASSERT_EQ(dropped_, true); }

  ~DropAsserter() {
    if (member_ != -1) {
      dropped_ = true;
    }
  }

 private:
  static std::atomic_bool dropped_;

  int member_{};
};

std::atomic_bool DropAsserter::dropped_ = false;

TEST(RuntimeDeathTest, drop_parameter) {
  EXPECT_EXIT(
      {
        auto rt = xyco::runtime::Builder::new_multi_thread()
                      .worker_threads(1)
                      .max_blocking_threads(1)
                      .build()
                      .unwrap();

        auto drop_asserter = DropAsserter(2);
        rt->spawn(
            [](DropAsserter drop_asserter) -> xyco::runtime::Future<void> {
              co_return;
            }(std::move(drop_asserter)));

        std::this_thread::sleep_for(time_deviation);

        DropAsserter::assert_drop();

        std::exit(0);
      },
      testing::ExitedWithCode(0), "");
}