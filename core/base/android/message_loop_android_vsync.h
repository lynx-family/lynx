// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_ANDROID_MESSAGE_LOOP_ANDROID_VSYNC_H_
#define CORE_BASE_ANDROID_MESSAGE_LOOP_ANDROID_VSYNC_H_

#include <memory>

#include "base/include/fml/platform/android/message_loop_android.h"
#include "core/base/android/vsync_monitor_android.h"
namespace lynx {
namespace base {
namespace android {
class MessageLoopAndroidVSync : public fml::MessageLoopAndroid {
 public:
  MessageLoopAndroidVSync();

  ~MessageLoopAndroidVSync() override = default;

  bool CanRunNow() override;

 private:
  void FlushTasks(fml::FlushType type) override;

  void WakeUp(fml::TimePoint time_point) override;

  bool WaitForVSyncTimeOut();

  bool HasPendingVSyncRequest();

  // The max execution time, its value is determined by the screen refresh rate.
  // Will be set inside the vsync callback.
  uint64_t max_execute_time_ms_ = 16;
  static constexpr uint64_t kNSecPerMSec = 1000000;
  // This is an estimated value. It indicates the proportion of FlushTasks in
  // the entire vsync cycle.
  static constexpr float kTraversalProportion = 0.75;
  // TODO(qiuxian): VSyncMonitor now locates in shell, needs to be moved to base
  std::shared_ptr<lynx::shell::VSyncMonitorAndroid> vsync_monitor_;

  // Used to record the time for requesting vsync, it will be reset to 0 when
  // the vsync callback is executed.
  uint64_t request_vsync_time_millis_ = 0;
  // The maximum timeout for waiting for the vsync callback.
  static constexpr uint64_t kWaitingVSyncTimeoutMillis = 5000;
};
}  // namespace android
}  // namespace base
}  // namespace lynx
#endif  // CORE_BASE_ANDROID_MESSAGE_LOOP_ANDROID_VSYNC_H_
