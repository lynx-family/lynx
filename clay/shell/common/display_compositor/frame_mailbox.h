// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_FRAME_MAILBOX_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_FRAME_MAILBOX_H_
#include <memory>
#include <mutex>

#include "clay/flow/compositor/compositor_frame.h"
namespace clay::display {
struct RasterPublication {
  std::shared_ptr<const PageSceneSnapshot> scene;
  std::shared_ptr<const RasterResult> result;
  explicit operator bool() const { return scene && result; }
};
class FrameMailbox {
 public:
  enum class PublishResult { kRejected, kCoalesced, kWake };
  explicit FrameMailbox(RasterBindingKey binding) : binding_(binding) {}
  PublishResult Publish(RasterPublication publication);
  RasterPublication TakeLatest();
  bool ReportFailure(PipelineError error);
  PipelineError TakeFailure();
  void Close();
  bool IsClosed();

 private:
  const RasterBindingKey binding_;
  std::mutex mutex_;
  uint64_t last_render_ = 0, last_layout_ = 0;
  bool closed_ = false, wake_pending_ = false;
  RasterPublication latest_;
  PipelineError error_ = PipelineError::kNone;
};
}  // namespace clay::display
#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_FRAME_MAILBOX_H_
