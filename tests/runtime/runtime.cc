#include "utils.h"

class TestLocalRegistry : public xyco::runtime::Registry {
 public:
  auto Register(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    event_ = event;
    return xyco::utils::Result<void>::ok();
  }

  auto reregister(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return xyco::utils::Result<void>::ok();
  }

  auto deregister(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return xyco::utils::Result<void>::ok();
  }

  auto select(xyco::runtime::Events &events, std::chrono::milliseconds timeout)
      -> xyco::utils::Result<void> override {
    if (event_) {
      events.push_back(event_);
      event_ = nullptr;
    }
    return xyco::utils::Result<void>::ok();
  }

 private:
  std::shared_ptr<xyco::runtime::Event> event_;
};

class TestGlobalRegistry : public xyco::runtime::GlobalRegistry {
 public:
  auto Register(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return register_local(event);
  }

  auto reregister(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return reregister_local(event);
  }

  auto deregister(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return deregister_local(event);
  }

  auto register_local(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return xyco::runtime::RuntimeCtx::get_ctx()
        ->driver()
        .local_handle<TestLocalRegistry>()
        ->Register(event);
  }

  auto reregister_local(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return xyco::runtime::RuntimeCtx::get_ctx()
        ->driver()
        .local_handle<TestLocalRegistry>()
        ->reregister(event);
  }

  auto deregister_local(std::shared_ptr<xyco::runtime::Event> event)
      -> xyco::utils::Result<void> override {
    return xyco::runtime::RuntimeCtx::get_ctx()
        ->driver()
        .local_handle<TestLocalRegistry>()
        ->deregister(event);
  }

  auto select(xyco::runtime::Events &events, std::chrono::milliseconds timeout)
      -> xyco::utils::Result<void> override {
    return xyco::utils::Result<void>::ok();
  }

  auto local_registry_init() -> void override {
    xyco::runtime::RuntimeCtx::get_ctx()
        ->driver()
        .add_local_registry<TestLocalRegistry>();
  }
};

class TestFuture : public xyco::runtime::Future<int> {
 public:
  TestFuture()
      : xyco::runtime::Future<int>(nullptr),
        event_(std::make_shared<xyco::runtime::Event>(
            xyco::runtime::Event{.future_ = this})) {}

  auto poll(xyco::runtime::Handle<void> self)
      -> xyco::runtime::Poll<int> override {
    if (!registered_) {
      registered_ = true;
      xyco::runtime::RuntimeCtx::get_ctx()
          ->driver()
          .Register<TestGlobalRegistry>(event_);
      return xyco::runtime::Pending();
    }
    return xyco::runtime::Ready<int>{1};
  }

 private:
  bool registered_{};
  std::shared_ptr<xyco::runtime::Event> event_;
};

TEST(RuntimeDeathTest, global_registry) {
  EXPECT_EXIT(
      {
        auto runtime = xyco::runtime::Builder::new_multi_thread()
                           .worker_threads(1)
                           .max_blocking_threads(1)
                           .registry<TestGlobalRegistry>()
                           .build()
                           .unwrap();

        runtime->spawn([]() -> xyco::runtime::Future<void> {
          auto result = co_await TestFuture();
          CO_ASSERT_EQ(result, 1);
          std::exit(0);
        }());

        while (true) {
          // Calls `sleep_for` to avoid while loop being optimized out.
          std::this_thread::sleep_for(wait_interval);
        }
      },
      testing::ExitedWithCode(0), "");
}
