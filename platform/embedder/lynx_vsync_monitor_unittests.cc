// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_vsync_monitor_priv.h"
#include "platform/embedder/public/lynx_vsync_monitor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::embedder {
namespace {

struct HostVSync {
  int requests = 0;
  lynx_vsync_monitor_on_vsync_callback signal = nullptr;
  void* context = nullptr;
  int completed = 0;
  uint64_t frame_id = 0;
  int64_t frame_start = 0;

  void Tick(int64_t start, int64_t target) {
    auto callback = signal;
    signal = nullptr;
    ASSERT_NE(callback, nullptr);
    callback(context, start, target);
  }
};

#if defined(ENABLE_FRAME_TICK_FENCE)
std::shared_ptr<LynxVSyncMonitor> CreateFencedMonitor(HostVSync& host) {
  auto* handle = lynx_vsync_monitor_create_with_finalizer(&host, nullptr);
  lynx_vsync_monitor_bind_request_vsync_func(
      handle, [](auto* monitor, auto callback, void* context) {
        auto* host =
            static_cast<HostVSync*>(lynx_vsync_monitor_get_user_data(monitor));
        ++host->requests;
        host->signal = callback;
        host->context = context;
      });
  lynx_vsync_monitor_bind_frame_tick_complete_func(
      handle, [](auto* monitor, uint64_t id, int64_t start) {
        auto* host =
            static_cast<HostVSync*>(lynx_vsync_monitor_get_user_data(monitor));
        ++host->completed;
        host->frame_id = id;
        host->frame_start = start;
      });
  return std::make_shared<LynxVSyncMonitor>(handle);
}

TEST(LynxVSyncMonitorTest, WaitsForWorkBeforeRequestingNextHostTick) {
  HostVSync host;
  auto monitor = CreateFencedMonitor(host);
  std::shared_ptr<base::VSyncFrameTickWork> work;
  monitor->RequestVSync([&](int64_t start, int64_t) {
    work = monitor->AcquireFrameTickWork(start);
    ASSERT_NE(work, nullptr);
    monitor->RequestVSync([](int64_t, int64_t) {});
  });
  host.Tick(100, 200);
  EXPECT_EQ(host.completed, 0);
  EXPECT_EQ(host.requests, 1);
  EXPECT_EQ(monitor->AcquireFrameTickWork(100), nullptr);
  work->Complete();
  EXPECT_EQ(host.completed, 1);
  EXPECT_EQ(host.frame_id, 1u);
  EXPECT_EQ(host.frame_start, 100);
  EXPECT_EQ(host.requests, 2);
  work->Complete();
  EXPECT_EQ(host.completed, 1);
  host.Tick(200, 300);
  EXPECT_EQ(host.completed, 2);
  EXPECT_EQ(host.frame_id, 2u);
}

TEST(LynxVSyncMonitorTest, WorkRetainsMonitorUntilCompletion) {
  HostVSync host;
  auto monitor = CreateFencedMonitor(host);
  std::weak_ptr<LynxVSyncMonitor> weak_monitor = monitor;
  std::shared_ptr<base::VSyncFrameTickWork> work;
  monitor->RequestVSync([&](int64_t start, int64_t) {
    work = monitor->AcquireFrameTickWork(start);
  });
  host.Tick(100, 200);
  ASSERT_NE(work, nullptr);
  monitor.reset();
  EXPECT_FALSE(weak_monitor.expired());
  work.reset();
  EXPECT_EQ(host.completed, 1);
  EXPECT_TRUE(weak_monitor.expired());
}

TEST(LynxVSyncMonitorTest, SynchronousHostCannotOverlapCallbackDispatch) {
  HostVSync host;
  auto monitor = CreateFencedMonitor(host);
  lynx_vsync_monitor_bind_request_vsync_func(
      monitor->GetCMonitor(), [](auto* monitor, auto callback, void* context) {
        auto* host =
            static_cast<HostVSync*>(lynx_vsync_monitor_get_user_data(monitor));
        const int tick = ++host->requests;
        callback(context, tick * 100, (tick + 1) * 100);
      });
  bool dispatching = false;
  int delivered = 0;
  monitor->RequestVSync([&](int64_t start, int64_t) {
    dispatching = true;
    EXPECT_EQ(start, 100);
    monitor->RequestVSync([&](int64_t start, int64_t) {
      EXPECT_FALSE(dispatching);
      EXPECT_EQ(start, 200);
      ++delivered;
    });
    EXPECT_EQ(host.requests, 1);
    dispatching = false;
    ++delivered;
  });
  EXPECT_EQ(delivered, 2);
  EXPECT_EQ(host.completed, 2);
}

TEST(LynxVSyncMonitorTest, PublicCompletionCallbackCanRequestNextTick) {
  class PublicMonitor final : public pub::LynxVSyncMonitor {
   public:
    PublicMonitor() : pub::LynxVSyncMonitor(nullptr) {}
    void RequestVSync(std::function<void(int64_t, int64_t)> callback) override {
      const int tick = ++requests;
      callback(tick * 100, (tick + 1) * 100);
    }
    int requests = 0;
  };
  auto host = std::make_shared<PublicMonitor>();
  host->InitIfNeed();
  auto monitor = std::make_shared<LynxVSyncMonitor>(host->GetCMonitor());
  int completed = 0;
  host->SetFrameTickCompleteCallback([&](uint64_t id, int64_t start) {
    ++completed;
    EXPECT_EQ(start, static_cast<int64_t>(id * 100));
    if (id == 1) {
      monitor->RequestVSync([](int64_t, int64_t) {});
      EXPECT_EQ(host->requests, 1);
    }
  });
  monitor->RequestVSync([](int64_t, int64_t) {});
  EXPECT_EQ(completed, 2);
  EXPECT_EQ(host->requests, 2);
}
#endif

TEST(LynxVSyncMonitorTest, FansOutOneHostSignalAndAllowsReentrantRequests) {
  HostVSync host;
  auto* handle = lynx_vsync_monitor_create_with_finalizer(&host, nullptr);
  lynx_vsync_monitor_bind_request_vsync_func(
      handle, [](auto* monitor, auto callback, void* context) {
        auto* host =
            static_cast<HostVSync*>(lynx_vsync_monitor_get_user_data(monitor));
        ++host->requests;
        host->signal = callback;
        host->context = context;
      });
  auto monitor = std::make_shared<LynxVSyncMonitor>(handle);
  int delivered = 0;
  monitor->RequestVSync([&](int64_t start, int64_t target) {
    EXPECT_EQ(start, 100);
    EXPECT_EQ(target, 200);
    ++delivered;
    monitor->RequestVSync([&](int64_t start, int64_t target) {
      EXPECT_EQ(start, 200);
      EXPECT_EQ(target, 300);
      ++delivered;
    });
  });
  monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  EXPECT_EQ(host.requests, 1);
  EXPECT_EQ(delivered, 0);
  host.Tick(100, 200);
  EXPECT_EQ(delivered, 2);
  EXPECT_EQ(host.requests, 2);
  host.Tick(200, 300);
  EXPECT_EQ(delivered, 3);
}

TEST(LynxVSyncMonitorTest, IgnoresLateAndDuplicateHostSignals) {
  HostVSync host;
  auto* handle = lynx_vsync_monitor_create_with_finalizer(&host, nullptr);
  lynx_vsync_monitor_bind_request_vsync_func(
      handle, [](auto* monitor, auto callback, void* context) {
        auto* host =
            static_cast<HostVSync*>(lynx_vsync_monitor_get_user_data(monitor));
        host->signal = callback;
        host->context = context;
      });
  auto monitor = std::make_shared<LynxVSyncMonitor>(handle);
  int delivered = 0;
  monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  auto old_signal = host.signal;
  auto old_context = host.context;
  host.Tick(100, 200);
  monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  old_signal(old_context, 100, 200);
  EXPECT_EQ(delivered, 1);
  auto late_signal = host.signal;
  auto late_context = host.context;
  monitor.reset();
  late_signal(late_context, 200, 300);
  EXPECT_EQ(delivered, 1);
}

}  // namespace
}  // namespace lynx::embedder
