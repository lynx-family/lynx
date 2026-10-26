// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public

#include "core/runtime/lepus/tasks/lepus_callback_manager.h"

#include <vector>

#include "base/include/value/base_value.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace air {
namespace testing {

class MockRuntime final : public runtime::MTSRuntime {
 public:
  MockRuntime()
      : runtime::MTSRuntime(runtime::ContextType::LepusNGContextType) {}

  lepus::Value CallClosureArgs(
      const lepus::Value& /*closure*/,
      const std::vector<lepus::Value>& /*args*/) override {
    invoked_ = true;
    return lepus::Value(1);
  }

  bool invoked_{false};
};

class MockMTSDelegate final : public runtime::MTSRuntime::Delegate {
 public:
  const std::string& TargetSdkVersion() override { return target_sdk_; }
  void ReportError(base::LynxError) override {}
  void OnBTSConsoleEvent(const std::string&, const std::string&) override {}
  void ReportGCTimingEvent(const char*, const char*) override {}
  void OnRuntimeGC(std::unordered_map<std::string, std::string>) override {}
  fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() override {
    return nullptr;
  }
  void OnScriptingStart() override {}
  void OnScriptingEnd() override {}
  void RequestVsync(
      uintptr_t id,
      base::MoveOnlyClosure<void, int64_t, int64_t> callback) override {
    callbacks_.emplace_back(id, std::move(callback));
  }

  std::string target_sdk_;
  std::vector<
      std::pair<uintptr_t, base::MoveOnlyClosure<void, int64_t, int64_t>>>
      callbacks_;
};

class LepusCallbackManagerTest : public ::testing::Test {
 public:
  LepusCallbackManagerTest() {}
  ~LepusCallbackManagerTest() override {}

  std::unique_ptr<tasm::LepusCallbackManager> lepus_callback_manager;
  MockRuntime context;

  void SetUp() override {
    lepus_callback_manager = std::make_unique<tasm::LepusCallbackManager>();
  }
};

TEST_F(LepusCallbackManagerTest, CacheTask) {
  int64_t id_1 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task1"));
  int64_t id_2 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task2"));
  int64_t id_3 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task3"));
  ASSERT_TRUE(id_1 == 1);
  ASSERT_TRUE(id_2 == 2);
  ASSERT_TRUE(id_3 == 3);
}

TEST_F(LepusCallbackManagerTest, InvokeTask) {
  int64_t id_1 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task1"));
  EXPECT_TRUE(id_1 == 1);
  std::vector<lepus::Value> args;
  args.emplace_back(lepus::Value(1));
  lepus_callback_manager->InvokeTask(id_1, args);
  EXPECT_TRUE(context.invoked_);
  EXPECT_TRUE(lepus_callback_manager->task_map_.empty());
  int64_t id_2 = lepus_callback_manager->CacheTask(
      &context, std::make_unique<lepus::Value>("task2"));
  EXPECT_TRUE(id_2 == 2);
  EXPECT_TRUE(!lepus_callback_manager->task_map_.empty());
}

TEST_F(LepusCallbackManagerTest, StateReleasedAfterDestroy) {
  std::weak_ptr<tasm::LepusCallbackManager::State> weak_state;

  {
    auto manager = std::make_unique<tasm::LepusCallbackManager>();
    weak_state = manager->state_;
    EXPECT_TRUE(weak_state.lock());
  }

  EXPECT_FALSE(weak_state.lock());
}

TEST_F(LepusCallbackManagerTest, WeakPtrPreventsExecutionAfterDestroy) {
  auto state = std::make_shared<tasm::LepusCallbackManager::State>();
  auto weak_state = std::weak_ptr<tasm::LepusCallbackManager::State>(state);

  bool executed = false;
  auto task = [weak_state, &executed]() {
    if (!weak_state.lock()) {
      return;
    }
    executed = true;
  };

  task();
  EXPECT_TRUE(executed);

  executed = false;
  state.reset();

  task();
  EXPECT_FALSE(executed);
}

TEST_F(LepusCallbackManagerTest, MTSRuntimeDefersAnimationFrameWhilePaused) {
  MockMTSDelegate delegate;
  context.SetDelegate(&delegate);
  context.SetPaused(true);

  int64_t received_frame_start = 0;
  auto callback = context.WrapAnimationFrameCallback(
      1, [&received_frame_start](int64_t frame_start, int64_t) {
        received_frame_start = frame_start;
      });
  callback(10, 20);
  EXPECT_EQ(received_frame_start, 0);

  context.SetPaused(false);
  ASSERT_EQ(delegate.callbacks_.size(), 1u);
  EXPECT_EQ(delegate.callbacks_[0].first, 1u);
  delegate.callbacks_[0].second(30, 40);
  EXPECT_EQ(received_frame_start, 30);
}

}  // namespace testing
}  // namespace air
}  // namespace lynx
