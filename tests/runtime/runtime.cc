#include "xyco/runtime/runtime.h"

#include <gtest/gtest.h>

class RuntimeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    runtime_ =
        *xyco::runtime::Builder::new_multi_thread().worker_threads(1).build();
  }

  static std::unique_ptr<xyco::runtime::Runtime> runtime_;
};

std::unique_ptr<xyco::runtime::Runtime> RuntimeTest::runtime_;

TEST_F(RuntimeTest, block_on) {
  runtime_->block_on([]() -> xyco::runtime::Future<void> { co_return; }());
  runtime_->block_on([]() -> xyco::runtime::Future<int> { co_return 1; }());
}
