#include "sync/mpsc.h"

#include <gtest/gtest.h>

#include "utils.h"

// TODO(dongxiaoyu): test cases for full and empty

TEST(MpscTest, one_sender) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    auto send_result = co_await sender.send(1);
    auto receive_result = co_await receiver.receive();

    CO_ASSERT_EQ(send_result.is_ok(), true);
    CO_ASSERT_EQ(receive_result.has_value(), true);
  }});
}

TEST(MpscTest, multi_sender) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 2>();
    co_await sender.send(1);

    auto another_sender = sender;
    another_sender.send(2);

    auto receive_result = co_await receiver.receive();
    CO_ASSERT_EQ(receive_result.value(), 1);

    receive_result = co_await receiver.receive();
    CO_ASSERT_EQ(receive_result.value(), 2);
  }});
}

TEST(MpscTest, receiver_close) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    receiver.xyco::sync::mpsc::Receiver<int, 1>::~Receiver();
    auto send_result = co_await sender.send(1);

    CO_ASSERT_EQ(send_result.unwrap_err(), 1);
  }});
}

TEST(MpscTest, sender_close) {
  TestRuntimeCtx::co_run({[]() -> xyco::runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::mpsc::channel<int, 1>();
    sender.xyco::sync::mpsc::Sender<int, 1>::~Sender();
    auto receive_result = co_await receiver.receive();

    CO_ASSERT_EQ(receive_result.has_value(), false);
  }});
}