#include "sync/oneshot.h"

#include <gtest/gtest.h>

#include "utils.h"

TEST(OneshotTest, success) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    auto send_result = co_await sender.send(1);
    auto value = (co_await receiver.receive()).unwrap();

    CO_ASSERT_EQ(send_result.is_ok(), true);
    CO_ASSERT_EQ(value, 1);
  }});
}

TEST(OneshotTest, sender_close) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    sender.xyco::sync::oneshot::Sender<int>::~Sender();
    auto value = (co_await receiver.receive());

    CO_ASSERT_EQ(value.is_err(), true);
  }});
}

TEST(OneshotTest, receiver_close) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    receiver.xyco::sync::oneshot::Receiver<int>::~Receiver();
    auto send_result = co_await sender.send(1);

    CO_ASSERT_EQ(send_result.unwrap_err(), 1);
  }});
}

TEST(OneshotTest, send_twice) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    co_await sender.send(1);
    auto send_twice_result = co_await sender.send(1);

    CO_ASSERT_EQ(send_twice_result.is_err(), true);
  }});
}

TEST(OneshotTest, receive_twice) {
  TestRuntimeCtx::co_run({[]() -> runtime::Future<void> {
    auto [sender, receiver] = xyco::sync::oneshot::channel<int>();
    co_await sender.send(1);
    co_await receiver.receive();
    auto receive_twice_result = co_await receiver.receive();

    CO_ASSERT_EQ(receive_twice_result.is_err(), true);
  }});
}