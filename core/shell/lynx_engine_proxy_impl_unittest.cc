// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_engine_proxy_impl.h"

#include <thread>
#include <utility>
#include <vector>

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/task_runner.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class MockTasmRunner : public fml::TaskRunner {
 public:
  MockTasmRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
  void PostDelayedTask(base::closure task, fml::TimeDelta delay) override {
    delays_.push_back(delay);
    delayed_tasks_.push_back(std::move(task));
  }

  void RunDelayedTasks() {
    auto tasks = std::move(delayed_tasks_);
    delayed_tasks_.clear();
    for (auto& task : tasks) {
      task();
    }
  }

  std::vector<fml::TimeDelta> delays_;
  std::vector<base::closure> delayed_tasks_;
};

class TestableLynxEngineProxyImpl : public LynxEngineProxyImpl {
 public:
  explicit TestableLynxEngineProxyImpl(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> actor)
      : LynxEngineProxyImpl(std::move(actor)) {}

  void ResetActor(
      const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& actor) {
    ResetEngineActor(actor);
  }
};

// Test that calling the LynxEngineProxy's API when the LynxEngine is empty does
// not cause a crash.
class LynxEngineProxyImplTest : public ::testing::Test {
 protected:
  LynxEngineProxyImplTest() = default;
  ~LynxEngineProxyImplTest() override = default;

  void SetUp() override {
    runner_ = fml::MakeRefCounted<MockTasmRunner>();
    engine_actor_ =
        std::make_shared<shell::LynxActor<shell::LynxEngine>>(nullptr, runner_);
    engine_proxy_ = std::make_unique<LynxEngineProxyImpl>(engine_actor_);
  }

  void TearDown() override {}

  fml::RefPtr<MockTasmRunner> runner_;
  std::shared_ptr<LynxActor<LynxEngine>> engine_actor_;
  std::unique_ptr<LynxEngineProxyImpl> engine_proxy_;
};

TEST_F(LynxEngineProxyImplTest, SendTouchEvent) {
  engine_proxy_->SendTouchEvent("test", 0, 0, 0, 0, 0, 0, 0);
}

TEST_F(LynxEngineProxyImplTest, SendTouchEventParams) {
  engine_proxy_->SendTouchEvent("test", pub::ValueImplLepus(lepus::Value()));
}

TEST_F(LynxEngineProxyImplTest, SendCustomEvent) {
  engine_proxy_->SendCustomEvent("test", 0, pub::ValueImplLepus(lepus::Value()),
                                 "test");
}

TEST_F(LynxEngineProxyImplTest, OnPseudoStatusChanged) {
  engine_proxy_->OnPseudoStatusChanged(0, 0, 0);
}

TEST_F(LynxEngineProxyImplTest, SendGestureEvent) {
  engine_proxy_->SendGestureEvent(1, 0, "test",
                                  pub::ValueImplLepus(lepus::Value()));
}

TEST_F(LynxEngineProxyImplTest, SendBubbleEvent) {
  engine_proxy_->SendBubbleEvent("test", 0,
                                 pub::ValueImplLepus(lepus::Value()));
}

TEST_F(LynxEngineProxyImplTest, UpdatePageCoordinateSnapshot) {
  engine_proxy_->UpdatePageCoordinateSnapshot(1.f, 2.f, true, 3.f, 4.f, true,
                                              false);
}

TEST_F(LynxEngineProxyImplTest, UpdateElementPositionState) {
  engine_proxy_->UpdateElementPositionState(
      {{1, ElementPositionUpdateType::kScrollOffset, 2.f, 3.f},
       {4, ElementPositionUpdateType::kStickyTranslation, 5.f, 6.f}});
}

TEST_F(LynxEngineProxyImplTest, PositionChangeTriggerUsesNonResettingWindow) {
  engine_proxy_->UpdateElementPositionState(
      {{1, ElementPositionUpdateType::kScrollOffset, 2.f, 3.f}});
  engine_proxy_->UpdatePageCoordinateSnapshot(1.f, 2.f, true, 3.f, 4.f, true,
                                              false);

  ASSERT_EQ(runner_->delayed_tasks_.size(), 1U);
  ASSERT_EQ(runner_->delays_.size(), 1U);
  EXPECT_EQ(runner_->delays_.front().ToMilliseconds(), 50);

  runner_->RunDelayedTasks();
  engine_proxy_->UpdateElementPositionState(
      {{1, ElementPositionUpdateType::kScrollOffset, 4.f, 5.f}});
  EXPECT_EQ(runner_->delayed_tasks_.size(), 1U);
}

TEST_F(LynxEngineProxyImplTest, DelayedTriggerDoesNotRetainDestroyedProxy) {
  engine_proxy_->UpdatePageCoordinateSnapshot(1.f, 2.f, true, 3.f, 4.f, true,
                                              false);
  ASSERT_EQ(runner_->delayed_tasks_.size(), 1U);
  engine_proxy_.reset();
  runner_->RunDelayedTasks();
}

TEST_F(LynxEngineProxyImplTest,
       ResetActorAllowsNewPositionChangeTriggerWhileOldWindowIsPending) {
  auto new_runner = fml::MakeRefCounted<MockTasmRunner>();
  auto new_actor = std::make_shared<shell::LynxActor<shell::LynxEngine>>(
      nullptr, new_runner);
  TestableLynxEngineProxyImpl proxy(engine_actor_);

  proxy.UpdateElementPositionState(
      {{1, ElementPositionUpdateType::kScrollOffset, 2.f, 3.f}});
  ASSERT_EQ(runner_->delayed_tasks_.size(), 1U);

  proxy.ResetActor(new_actor);
  proxy.UpdateElementPositionState(
      {{1, ElementPositionUpdateType::kScrollOffset, 4.f, 5.f}});
  EXPECT_EQ(new_runner->delayed_tasks_.size(), 1U);

  runner_->RunDelayedTasks();
  EXPECT_EQ(new_runner->delayed_tasks_.size(), 1U);
}

TEST_F(LynxEngineProxyImplTest, EnableRasterAnimation) {
  engine_proxy_->EnableRasterAnimation();
}

}  // namespace shell
}  // namespace lynx
