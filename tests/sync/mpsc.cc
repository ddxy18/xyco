#include <gtest/gtest.h>

#include <coroutine>
#include <thread>

import xyco.test.utils;
import xyco.sync;

TEST(MpscTest, receive_moveonly) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<MoveOnlyObject, 1>();
    auto send_result = co_await sender.send({});
    auto receive_result = co_await receiver.receive();

    CO_ASSERT_EQ(send_result.has_value(), true);
    CO_ASSERT_EQ(*receive_result, MoveOnlyObject());
  }());
}

TEST(MpscTest, one_sender) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    auto send_result = co_await sender.send(1);
    auto receive_result = co_await receiver.receive();

    CO_ASSERT_EQ(send_result.has_value(), true);
    CO_ASSERT_EQ(receive_result.has_value(), true);
  }());
}

TEST(MpscTest, multi_sender) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 2>();
    co_await sender.send(1);

    auto another_sender = sender;
    co_await another_sender.send(2);

    auto receive_result = co_await receiver.receive();
    CO_ASSERT_EQ(*receive_result, 1);

    receive_result = co_await receiver.receive();
    CO_ASSERT_EQ(*receive_result, 2);
  }());
}

TEST(MpscTest, receiver_close) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    {
      auto tmp_receiver =
          std::move(receiver);  // Destructed early to mock receiver closed.
    }
    auto send_result = co_await sender.send(1);

    CO_ASSERT_EQ(send_result.error(), 1);
  }());
}

TEST(MpscTest, sender_close) {
  TestRuntimeCtx::runtime()->block_on([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    {
      auto tmp_sender =
          std::move(sender);  // Destructed early to mock sender closed.
    }
    auto receive_result = co_await receiver.receive();

    CO_ASSERT_EQ(receive_result.has_value(), false);
  }());
}

TEST(MpscTest, send_to_full_channel) {
  auto channel_pair = xyco::sync::mpsc::channel<int, 1>();

  int value = 0;
  auto send_result = std::expected<void, int>();

  std::thread send([&]() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      co_await channel_pair.first.send(1);
      send_result = co_await channel_pair.first.send(2);
    }());
  });

  std::this_thread::sleep_for(wait_interval);  // wait sender pending

  std::thread receive([&]() {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      value = *co_await channel_pair.second.receive();
    }());
  });

  send.join();
  receive.join();

  ASSERT_EQ(value, 1);
  ASSERT_EQ(send_result.has_value(), true);
}

TEST(MpscTest, receive_from_empty_channel) {
  auto channel_pair = xyco::sync::mpsc::channel<int, 2>();

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
