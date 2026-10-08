// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/tasks/lepus_raf_manager.h"

#include <memory>
#include <unordered_set>
#include <vector>

#include "core/base/threading/vsync_monitor.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
namespace {

class FrameRateMonitor : public base::VSyncMonitor {
 public:
  void SetAnimationFrameRate(uintptr_t client_id, bool active) override {
    states.push_back(active);
    if (active) {
      clients.insert(client_id);
    } else {
      clients.erase(client_id);
    }
  }

  std::vector<bool> states;
  std::unordered_set<uintptr_t> clients;
};

int64_t RequestFrame(AnimationFrameManager& manager) {
  // These lifecycle tests cancel every closure before attempting execution.
  return manager.RequestAnimationFrame(nullptr,
                                       std::make_unique<lepus::Value>());
}

TEST(AnimationFrameRateTest, FirstRequestAndLastCancellation) {
  auto monitor = std::make_shared<FrameRateMonitor>();
  AnimationFrameManager manager;
  manager.SetVSyncMonitor(monitor);
  auto first = RequestFrame(manager);
  auto second = RequestFrame(manager);
  ASSERT_EQ(monitor->states, std::vector<bool>({true}));

  manager.CancelAnimationFrame(first);
  manager.CancelAnimationFrame(first);
  manager.CancelAnimationFrame(-1);
  EXPECT_TRUE(manager.HasPendingRequest());
  EXPECT_EQ(monitor->states, std::vector<bool>({true}));

  manager.CancelAnimationFrame(second);
  EXPECT_FALSE(manager.HasPendingRequest());
  EXPECT_EQ(monitor->states, std::vector<bool>({true, false}));
  manager.DoFrame(0);
  EXPECT_EQ(monitor->states, std::vector<bool>({true, false}));
}

TEST(AnimationFrameRateTest, NewRequestAfterCancellation) {
  auto monitor = std::make_shared<FrameRateMonitor>();
  AnimationFrameManager manager;
  manager.SetVSyncMonitor(monitor);
  manager.CancelAnimationFrame(RequestFrame(manager));
  manager.CancelAnimationFrame(RequestFrame(manager));
  EXPECT_EQ(monitor->states, std::vector<bool>({true, false, true, false}));
}

TEST(AnimationFrameRateTest, DestroyReleasesOnlyItsOwnClient) {
  auto monitor = std::make_shared<FrameRateMonitor>();
  AnimationFrameManager first, second;
  first.SetVSyncMonitor(monitor);
  second.SetVSyncMonitor(monitor);
  RequestFrame(first);
  RequestFrame(second);
  ASSERT_EQ(monitor->clients.size(), 2u);

  first.Destroy();
  EXPECT_FALSE(first.HasPendingRequest());
  EXPECT_TRUE(second.HasPendingRequest());
  EXPECT_EQ(monitor->clients.size(), 1u);
  second.Destroy();
  EXPECT_TRUE(monitor->clients.empty());
}

TEST(AnimationFrameRateTest, RebindingTransfersAnActivePreference) {
  auto first = std::make_shared<FrameRateMonitor>();
  auto second = std::make_shared<FrameRateMonitor>();
  AnimationFrameManager manager;
  manager.SetVSyncMonitor(first);
  RequestFrame(manager);
  manager.SetVSyncMonitor(first);
  EXPECT_EQ(first->states, std::vector<bool>({true}));
  manager.SetVSyncMonitor(second);
  EXPECT_TRUE(first->clients.empty());
  EXPECT_EQ(second->clients.size(), 1u);
  manager.Destroy();
  EXPECT_TRUE(second->clients.empty());
}

TEST(AnimationFrameRateTest, ManagerDoesNotKeepTheMonitorAlive) {
  auto monitor = std::make_shared<FrameRateMonitor>();
  std::weak_ptr<FrameRateMonitor> weak = monitor;
  AnimationFrameManager manager;
  manager.SetVSyncMonitor(monitor);
  RequestFrame(manager);
  monitor.reset();
  EXPECT_TRUE(weak.expired());
  manager.Destroy();
  EXPECT_FALSE(manager.HasPendingRequest());
}

TEST(AnimationFrameRateTest, RuntimeTeardownCancelsARetainedManager) {
  auto monitor = std::make_shared<FrameRateMonitor>();
  auto runtime = std::make_unique<runtime::MTSRuntime>(
      runtime::ContextType::LepusNGContextType);
  auto manager = runtime->GetAnimationFrameManager();
  manager->SetVSyncMonitor(monitor);
  manager->RequestAnimationFrame(runtime.get(),
                                 std::make_unique<lepus::Value>());
  ASSERT_TRUE(manager->HasPendingRequest());

  // Model the strong manager reference held by a queued VSync callback.
  runtime.reset();
  EXPECT_FALSE(manager->HasPendingRequest());
  EXPECT_TRUE(monitor->clients.empty());
  manager->DoFrame(0);
}

}  // namespace
}  // namespace testing
}  // namespace tasm
}  // namespace lynx
