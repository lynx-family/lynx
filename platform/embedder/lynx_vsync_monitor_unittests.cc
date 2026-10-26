// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_vsync_monitor_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::embedder {
namespace {

struct HostVSync {
  int requests = 0;
  lynx_vsync_monitor_on_vsync_callback signal = nullptr;
  void* context = nullptr;

  void Tick(int64_t start, int64_t target) {
    auto callback = signal;
    signal = nullptr;
    ASSERT_NE(callback, nullptr);
    callback(context, start, target);
  }
};

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
