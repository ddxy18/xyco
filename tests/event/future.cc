#include <gtest/gtest.h>

#include <gsl/pointers>
#include <memory>

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
    runtime::RuntimeCtx::set_ctx(rt_.get());
  }

 private:
  std::unique_ptr<runtime::Runtime> rt_;
};

TEST_F(InRuntimeTest, NoSuspend) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    int co_result = -1;
    auto co_innner = []() -> runtime::Future<int> { co_return 1; };
    co_result = co_await co_innner();

    CO_ASSERT_EQ(co_result, 1);
  }});
}

TEST_F(InRuntimeTest, Suspend) {
  int co_result = -1;
  runtime::Future<int> *handle = nullptr;
  auto co_outer =
      TestRuntimeCtx::co_run_no_wait({[&]() -> runtime::Future<void> {
        auto co_innner = [&]() -> runtime::Future<int> {
          class SuspendFuture : public runtime::Future<int> {
           public:
            SuspendFuture(runtime::Future<int> *&handle)
                : runtime::Future<int>(nullptr), handle_(handle) {}

            [[nodiscard]] auto poll(runtime::Handle<void> self)
                -> runtime::Poll<int> override {
              if (!ready_) {
                ready_ = true;
                handle_ = this;
                return runtime::Pending();
              }
              return runtime::Ready<int>{1};
            }

           private:
            bool ready_{false};
            runtime::Future<int> *&handle_;
          };
          co_return co_await SuspendFuture(handle);
        };
        co_result = co_await co_innner();
      }});
  if (handle->poll_wrapper()) {
    handle->get_handle().resume();
  }

  ASSERT_EQ(co_result, 1);
}

TEST_F(NoRuntimeTest, NeverRun) {
  int co_result = -1;
  auto co_outer =
      TestRuntimeCtx::co_run_without_runtime({[&]() -> runtime::Future<void> {
        auto co_innner = []() -> runtime::Future<int> { co_return 1; };
        co_result = co_await co_innner();
      }});
      
  ASSERT_EQ(co_result, -1);
}