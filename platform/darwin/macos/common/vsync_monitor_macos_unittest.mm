// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AppKit/AppKit.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <thread>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/thread.h"
#include "core/base/threading/vsync_monitor.h"
#include "platform/darwin/macos/common/vsync_monitor_macos.h"
#include "platform/embedder/windowless/windowless_ui_task_runner_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace {

// Construct the platform implementation directly: the shared unit-test factory
// intentionally returns nullptr and must remain unchanged for other tests.
std::shared_ptr<VSyncMonitor> CreateMacOSMonitor() {
  if (@available(macOS 14.0, *)) {
    return std::make_shared<VSyncMonitorMacOS>();
  }
  return nullptr;
}

class VSyncMonitorMacOSTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (@available(macOS 14.0, *)) {
      return;
    }
    GTEST_SKIP() << "VSyncMonitorMacOS requires macOS 14 or later";
  }
};

// Exercise the real windowless delegate without initializing or pumping a
// native message loop. Only the host's queued tasks may deliver frame callbacks.
class HostQueue {
 public:
  HostQueue() : delegate_(Config()), runner_(fml::MakeRefCounted<fml::TaskRunner>(nullptr)) {
    runner_->SetDelegate(&delegate_);
  }

  lynx_windowless_ui_task_runner_config_t Config() {
    lynx_windowless_ui_task_runner_config_t config{};
    config.struct_size = sizeof(config);
    config.user_data = this;
    config.runs_on_current_thread_callback = [](void* data) {
      return static_cast<HostQueue*>(data)->owner_ == std::this_thread::get_id();
    };
    config.post_task_callback = [](lynx_task_t task, uint64_t target, void* data) {
      static_cast<HostQueue*>(data)->tasks_.push_back({task, target});
    };
    return config;
  }

  bool RunNext() {
    if (tasks_.empty()) return false;
    auto pending = tasks_.front();
    tasks_.pop_front();
    auto now = fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
    // Fail instead of hanging if a unit conversion schedules far into the future.
    if (pending.target > static_cast<uint64_t>(now) + 1000000000ULL) {
      ADD_FAILURE() << "Frame task scheduled more than one second ahead";
      return false;
    }
    if (pending.target > static_cast<uint64_t>(now)) {
      std::this_thread::sleep_for(std::chrono::nanoseconds(pending.target - now));
    }
    return delegate_.RunTask(pending.task.task);
  }

  std::shared_ptr<lynx::base::VSyncMonitor> CreateMonitor() {
    auto monitor = CreateMacOSMonitor();
    monitor->BindTaskRunner(runner_);
    monitor->Init();
    return monitor;
  }

  struct Pending {
    lynx_task_t task;
    uint64_t target;
  };
  const std::thread::id owner_ = std::this_thread::get_id();
  std::deque<Pending> tasks_;
  lynx::embedder::WindowlessUITaskRunnerDelegate delegate_;
  fml::RefPtr<fml::TaskRunner> runner_;
};

TEST_F(VSyncMonitorMacOSTest, SecondaryCallbacksContinueWithoutNSRunLoop) {
  HostQueue host;
  ASSERT_FALSE(host.runner_->GetLoop());
  auto monitor = host.CreateMonitor();
  int frames = 0;
  int64_t previous = 0;
  std::function<void()> request;
  request = [&] {
    monitor->ScheduleVSyncSecondaryCallback(1, [&](int64_t start, int64_t end) {
      EXPECT_GT(end, start);
      EXPECT_GT(start, previous);
      previous = start;
      if (++frames < 12) request();
    });
  };
  request();
  EXPECT_EQ(frames, 0);
  for (int i = 0; i < 12; ++i) ASSERT_TRUE(host.RunNext());
  EXPECT_EQ(frames, 12);
  EXPECT_TRUE(host.tasks_.empty());
}

TEST_F(VSyncMonitorMacOSTest, PrimaryCallbacksContinueWithoutNSRunLoop) {
  HostQueue host;
  auto monitor = host.CreateMonitor();
  int frames = 0;
  std::function<void()> request;
  request = [&] {
    monitor->AsyncRequestVSync([&](int64_t start, int64_t end) {
      EXPECT_GT(end, start);
      if (++frames < 4) request();
    });
  };
  request();
  EXPECT_EQ(frames, 0);
  for (int i = 0; i < 4; ++i) ASSERT_TRUE(host.RunNext());
  EXPECT_EQ(frames, 4);
}

TEST_F(VSyncMonitorMacOSTest, SecondaryCallbacksAreCoalesced) {
  HostQueue host;
  auto monitor = host.CreateMonitor();
  int calls = 0;
  monitor->ScheduleVSyncSecondaryCallback(1, [&](int64_t, int64_t) { ++calls; });
  monitor->ScheduleVSyncSecondaryCallback(1, [&](int64_t, int64_t) { ADD_FAILURE(); });
  monitor->ScheduleVSyncSecondaryCallback(2, [&](int64_t, int64_t) { ++calls; });
  ASSERT_EQ(host.tasks_.size(), 1u);
  ASSERT_TRUE(host.RunNext());
  EXPECT_EQ(calls, 2);
  EXPECT_TRUE(host.tasks_.empty());
}

TEST_F(VSyncMonitorMacOSTest, DestructionCancelsPendingFrame) {
  HostQueue host;
  auto monitor = host.CreateMonitor();
  monitor->ScheduleVSyncSecondaryCallback(1, [&](int64_t, int64_t) { ADD_FAILURE(); });
  ASSERT_EQ(host.tasks_.size(), 1u);
  monitor.reset();
  ASSERT_TRUE(host.RunNext());
  EXPECT_TRUE(host.tasks_.empty());
}

TEST_F(VSyncMonitorMacOSTest, StopCancelsPendingFrame) {
  HostQueue host;
  auto monitor = host.CreateMonitor();
  monitor->ScheduleVSyncSecondaryCallback(1, [&](int64_t, int64_t) { ADD_FAILURE(); });
  monitor->StopVSync();
  ASSERT_TRUE(host.RunNext());
  EXPECT_TRUE(host.tasks_.empty());
}

// These integration checks require a live WindowServer/display. Opt in with
// --gtest_also_run_disabled_tests --gtest_filter='*DISABLED_*' on a desktop host.
TEST_F(VSyncMonitorMacOSTest, DISABLED_NativeRunLoopStillDeliversFrames) {
  [NSApplication sharedApplication];
  [NSApp finishLaunching];
  auto& loop = fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto monitor = CreateMacOSMonitor();
  monitor->BindTaskRunner(loop.GetTaskRunner());
  monitor->Init();
  int frames = 0;
  std::function<void()> request;
  request = [&] {
    monitor->ScheduleVSyncSecondaryCallback(1, [&](int64_t start, int64_t end) {
      EXPECT_GT(end, start);
      if (++frames < 3) request();
    });
  };
  request();
  NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:2];
  while (frames < 3 && [deadline timeIntervalSinceNow] > 0) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
  }
  EXPECT_EQ(frames, 3);
}
bool PumpNativeUntil(const std::function<bool()>& ready) {
  NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:2];
  while (!ready() && [deadline timeIntervalSinceNow] > 0) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
  }
  return ready();
}

TEST_F(VSyncMonitorMacOSTest, DISABLED_NativeBackgroundLoopDeliversFrames) {
  [NSApplication sharedApplication];
  [NSApp finishLaunching];
  fml::Thread worker("vsync-background-review");
  auto runner = worker.GetTaskRunner();
  std::shared_ptr<lynx::base::VSyncMonitor> monitor;
  std::atomic<int> frames{0};
  std::function<void()> request;
  request = [&] {
    monitor->ScheduleVSyncSecondaryCallback(101, [&](int64_t start, int64_t end) {
      EXPECT_TRUE(runner->RunsTasksOnCurrentThread());
      EXPECT_GT(end, start);
      if (++frames < 6) request();
    });
  };
  runner->PostSyncTask([&] {
    monitor = CreateMacOSMonitor();
    monitor->BindTaskRunner(runner);
    monitor->Init();
    request();
  });
  EXPECT_TRUE(PumpNativeUntil([&] { return frames >= 6; }));
  runner->PostSyncTask([&] { monitor.reset(); });
}

TEST_F(VSyncMonitorMacOSTest, DISABLED_NativeRunnerMigrationPreservesCallbacks) {
  [NSApplication sharedApplication];
  [NSApp finishLaunching];
  auto& main_loop = fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto runner = fml::MakeRefCounted<fml::TaskRunner>(main_loop.GetLoopImpl());
  fml::Thread worker("vsync-migration-review");
  auto monitor = CreateMacOSMonitor();
  monitor->BindTaskRunner(runner);
  monitor->Init();
  std::atomic<int> frames{0};
  std::atomic<bool> expected_background{false};
  std::function<void()> request;
  request = [&] {
    monitor->ScheduleVSyncSecondaryCallback(102, [&](int64_t start, int64_t end) {
      EXPECT_TRUE(runner->RunsTasksOnCurrentThread());
      EXPECT_EQ(worker.GetTaskRunner()->RunsTasksOnCurrentThread(), expected_background.load());
      EXPECT_GT(end, start);
      if (++frames % 3 != 0) request();
    });
  };
  request();
  EXPECT_TRUE(PumpNativeUntil([&] { return frames >= 3; }));
  runner->UnBind();
  expected_background = true;
  worker.GetTaskRunner()->PostSyncTask([&] {
    runner->Bind(worker.GetLoop());
    request();
  });
  EXPECT_TRUE(PumpNativeUntil([&] { return frames >= 6; }));
  worker.GetTaskRunner()->PostSyncTask([&] { runner->UnBind(); });
  expected_background = false;
  runner->Bind(main_loop.GetLoopImpl());
  request();
  EXPECT_TRUE(PumpNativeUntil([&] { return frames >= 9; }));
  monitor.reset();
}
}  // namespace

}  // namespace base
}  // namespace lynx
