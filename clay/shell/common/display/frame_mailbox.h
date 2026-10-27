// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_FRAME_MAILBOX_H_
#define CLAY_SHELL_COMMON_DISPLAY_FRAME_MAILBOX_H_
#include <memory>
#include <mutex>

#include "clay/flow/compositor/compositor_frame.h"
namespace clay::display {
class FrameMailbox {
 public:
  enum class PublishResult { kRejected, kCoalesced, kWake };
  explicit FrameMailbox(RasterBindingKey binding) : binding_(binding) {}
  PublishResult Publish(const PageSceneSnapshot& scene,
                        std::shared_ptr<const RasterResult> result);
  std::shared_ptr<const RasterResult> TakeLatest();
  bool ReportFailure(PipelineError error);
  PipelineError TakeFailure();
  void Close();

 private:
  const RasterBindingKey binding_;
  std::mutex mutex_;
  uint64_t last_render_ = 0, last_layout_ = 0;
  bool closed_ = false, wake_pending_ = false;
  std::shared_ptr<const RasterResult> latest_;
  PipelineError error_ = PipelineError::kNone;
};
}  // namespace clay::display
#endif  // CLAY_SHELL_COMMON_DISPLAY_FRAME_MAILBOX_H_
