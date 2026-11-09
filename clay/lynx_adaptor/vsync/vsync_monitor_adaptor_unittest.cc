// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/vsync/vsync_monitor_adaptor.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "core/public/vsync_monitor_platform_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {
namespace {

class RecordingVSyncMonitor final
    : public lynx::base::VSyncMonitorPlatformImpl {
 public:
  void RequestVSync(Callback callback) override {
    ++request_count;
    pending_callback = std::move(callback);
  }

  int request_count = 0;
  Callback pending_callback;
};

TEST(VSyncMonitorAdaptorTest, NullSourceLeavesSystemVsyncAvailable) {
  EXPECT_FALSE(MakeVsyncRequestCallback(nullptr));
}

TEST(VSyncMonitorAdaptorTest, ForwardsRequestAndAsynchronousNanosecondSignal) {
  auto monitor = std::make_shared<RecordingVSyncMonitor>();
  auto request = MakeVsyncRequestCallback(monitor);
  int delivered = 0;
  constexpr int64_t kFrameStart = 4'123'456'789'000;
  constexpr int64_t kFrameTarget = kFrameStart + 16'666'667;
  request([&](int64_t frame_start_time_ns, int64_t frame_target_time_ns) {
    EXPECT_EQ(frame_start_time_ns, kFrameStart);
    EXPECT_EQ(frame_target_time_ns, kFrameTarget);
    ++delivered;
  });

  EXPECT_EQ(monitor->request_count, 1);
  EXPECT_EQ(delivered, 0);
  auto reply = std::move(monitor->pending_callback);
  ASSERT_TRUE(static_cast<bool>(reply));
  reply(kFrameStart, kFrameTarget);
  EXPECT_EQ(delivered, 1);
}

TEST(VSyncMonitorAdaptorTest, LastRequestCallbackCopyReleasesSource) {
  auto monitor = std::make_shared<RecordingVSyncMonitor>();
  std::weak_ptr<RecordingVSyncMonitor> weak_monitor = monitor;
  auto request = MakeVsyncRequestCallback(monitor);
  auto request_copy = request;
  monitor.reset();
  EXPECT_FALSE(weak_monitor.expired());

  request = {};
  EXPECT_FALSE(weak_monitor.expired());
  request_copy = {};
  EXPECT_TRUE(weak_monitor.expired());
}

}  // namespace
}  // namespace testing
}  // namespace clay
