#include "event/runtime/future.h"

#include <gtest/gtest.h>

#include <functional>
#include <gsl/pointers>

#include "event/runtime/runtime.h"

class InRuntimeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rt_ = runtime::Builder().build().unwrap();
    runtime::RuntimeCtx::set_ctx(rt_.get());
  }

 private:
  std::unique_ptr<runtime::Runtime> rt_;
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

// co_outer's lifetime is managed by the caller to avoid being destroyed
// automatically before resumed.
template <typename T>
auto co_run(std::function<runtime::Future<T>()> &&co, T &co_result)
    -> gsl::owner<std::function<runtime::Future<void>()> *> {
  auto *co_outer = gsl::owner<std::function<runtime::Future<void>()> *>(
      new std::function<runtime::Future<void>()>(
          [&]() -> runtime::Future<void> { co_result = co_await co(); }));
  (*co_outer)();

  return co_outer;
}

TEST_F(InRuntimeTest, NoSuspend) {
  int co_result = -1;
  gsl::owner<std::function<runtime::Future<void>()> *> co_outer =
      co_run({[]() -> runtime::Future<int> { co_return 1; }}, co_result);
  delete co_outer;

  ASSERT_EQ(co_result, 1);
}

TEST_F(InRuntimeTest, Suspend) {
  int co_result = -1;
  runtime::Future<int> *handle = nullptr;
  gsl::owner<std::function<runtime::Future<void>()> *> co_outer =
      co_run({[&]() -> runtime::Future<int> {
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
             }},
             co_result);
  if (handle->poll_wrapper()) {
    handle->get_handle().resume();
  }
  delete co_outer;

  ASSERT_EQ(co_result, 1);
}

TEST_F(NoRuntimeTest, NeverRun) {
  int co_result = -1;
  gsl::owner<std::function<runtime::Future<void>()> *> co_outer =
      co_run({[]() -> runtime::Future<int> { co_return 1; }}, co_result);
  delete co_outer;

  ASSERT_EQ(co_result, -1);
}