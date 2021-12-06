#include "sync/mpsc.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(MpscTest, one_sender) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    auto send_result = co_await sender.send(1);
    auto receive_result = co_await receiver.receive();

    CO_ASSERT_EQ(send_result.is_ok(), true);
    CO_ASSERT_EQ(receive_result.has_value(), true);
  });
}

TEST(MpscTest, multi_sender) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 2>();
    co_await sender.send(1);

    auto another_sender = sender;
    co_await another_sender.send(2);

    auto receive_result = co_await receiver.receive();
    CO_ASSERT_EQ(receive_result.value(), 1);

    receive_result = co_await receiver.receive();
    CO_ASSERT_EQ(receive_result.value(), 2);
  });
}

TEST(MpscTest, receiver_close) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    receiver.xyco::sync::mpsc::Receiver<int, 1>::~Receiver();
    auto send_result = co_await sender.send(1);

    CO_ASSERT_EQ(send_result.unwrap_err(), 1);
  });
}

TEST(MpscTest, sender_close) {
  TestRuntimeCtx::co_run([]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    sender.xyco::sync::mpsc::Sender<int, 1>::~Sender();
    auto receive_result = co_await receiver.receive();

    CO_ASSERT_EQ(receive_result.has_value(), false);
  });
}

TEST(MpscTest, send_to_full_channel) {
  auto channel_pair = xyco::sync::mpsc::channel<int, 1>();

  int value = 0;
  Result<void, int> send_result;

  std::thread send([&]() {
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      co_await channel_pair.first.send(1);
      send_result = co_await channel_pair.first.send(2);
    });
  });

  std::this_thread::sleep_for(time_deviation);  // wait sender pending

  std::thread receive([&]() {
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      value = (co_await channel_pair.second.receive()).value();
    });
  });

  send.join();
  receive.join();

  ASSERT_EQ(value, 1);
  ASSERT_EQ(send_result.is_ok(), true);
}

TEST(MpscTest, receive_from_empty_channel) {
  auto channel_pair = xyco::sync::mpsc::channel<int, 2>();

  int value = 0;
  Result<void, int> send_result;

  std::thread receive([&]() {
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      value = (co_await channel_pair.second.receive()).value();
    });
  });

  std::this_thread::sleep_for(time_deviation);  // wait receive pending

  std::thread send([&]() {
    TestRuntimeCtx::co_run([&]() -> xyco::runtime::Future<void> {
      send_result = co_await channel_pair.first.send(1);
    });
  });

  receive.join();
  send.join();

  ASSERT_EQ(value, 1);
  ASSERT_EQ(send_result.is_ok(), true);
}