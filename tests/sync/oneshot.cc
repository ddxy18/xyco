#include "xyco/sync/oneshot.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <utility>

#include "utils.h"

TEST(OneshotTest, success) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    auto send_result = co_await sender.send(1);
    auto value = *co_await receiver.receive();

    CO_ASSERT_EQ(send_result.has_value(), true);
    CO_ASSERT_EQ(value, 1);
  }());
}

TEST(OneshotTest, sender_close) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    {
      auto tmp_sender =
          std::move(sender);  // Destructed early to mock sender closed.
    }
    auto value = (co_await receiver.receive());

    CO_ASSERT_EQ(value.has_value(), false);
  }());
}

TEST(OneshotTest, receiver_close) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    {
      auto tmp_receiver =
          std::move(receiver);  // Destructed early to mock receiver closed.
    }
    auto send_result = co_await sender.send(1);

    CO_ASSERT_EQ(send_result.error(), 1);
  }());
}

TEST(OneshotTest, send_twice) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    co_await sender.send(1);
    auto send_twice_result = co_await sender.send(1);

    CO_ASSERT_EQ(send_twice_result.has_value(), false);
  }());
}

TEST(OneshotTest, receive_twice) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    co_await sender.send(1);
    co_await receiver.receive();
    auto receive_twice_result = co_await receiver.receive();

    CO_ASSERT_EQ(receive_twice_result.has_value(), false);
  }());
}

TEST(OneshotTest, receive_first) {
  auto channel_pair = xyco::sync::oneshot::channel<int>();

  int value = 0;
  auto send_result = std::expected<void, int>();

  std::thread receive([&]() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      value = *co_await channel_pair.second.receive();
    }());
  });

  std::this_thread::sleep_for(wait_interval);  // wait receive pending

  std::thread send([&]() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      send_result = co_await channel_pair.first.send(1);
    }());
  });

  receive.join();
  send.join();

  ASSERT_EQ(value, 1);
  ASSERT_EQ(send_result.has_value(), true);
}
