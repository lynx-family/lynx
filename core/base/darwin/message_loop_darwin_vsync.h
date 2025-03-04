// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_DARWIN_MESSAGE_LOOP_DARWIN_VSYNC_H_
#define CORE_BASE_DARWIN_MESSAGE_LOOP_DARWIN_VSYNC_H_

#include <memory>

#include "core/shell/ios/vsync_monitor_darwin.h"
#include "lynx/base/include/fml/platform/darwin/message_loop_darwin.h"
namespace lynx {
namespace base {
namespace darwin {
class MessageLoopDarwinVSync : public fml::MessageLoopDarwin {
 public:
  MessageLoopDarwinVSync();

  ~MessageLoopDarwinVSync() override = default;

  bool CanRunNow() override;

 private:
  void WakeUp(fml::TimePoint time_point) override;

  bool HasPendingVSyncRequest();
  uint64_t max_execute_time_ms_ = 16;
  static constexpr uint64_t kNSecPerMSec = 1000000;
  // TODO(qiuxian): VSyncMonitor now locates in shell, needs to be moved to base
  std::shared_ptr<lynx::shell::VSyncMonitorIOS> vsync_monitor_;
  // Used to record the time for requesting vsync, it will be reset to 0 when
  // the vsync callback is executed.
  uint64_t request_vsync_time_millis_ = 0;
};
}  // namespace darwin
}  // namespace base
}  // namespace lynx
#endif  // CORE_BASE_DARWIN_MESSAGE_LOOP_DARWIN_VSYNC_H_
