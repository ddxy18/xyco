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
  int co_result = -1;
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
  int co_result = -1;
  auto co_outer = TestRuntimeCtx::co_run_without_runtime(
      [&]() -> xyco::runtime::Future<void> {
        auto co_innner = []() -> xyco::runtime::Future<int> { co_return 1; };
        co_result = co_await co_innner();
      });

  ASSERT_EQ(co_result, -1);
}

TEST(InRuntimeDeathTest, coroutine_exception) {
  auto rt = xyco::runtime::Builder::new_multi_thread()
                .worker_threads(2)
                .max_blocking_threads(1)
                .build()
                .unwrap();
  rt->spawn([]() -> xyco::runtime::Future<void> {
    throw std::runtime_error("");
    co_return;
  }());

  auto result = -1;
  auto fut = [&]() -> xyco::runtime::Future<void> {
    result = 1;
    co_return;
  };
  rt->spawn(fut());
  std::this_thread::sleep_for(time_deviation);

  ASSERT_EQ(result, 1);
}