// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/frame_mailbox.h"

#include <utility>
namespace clay::display {
FrameMailbox::PublishResult FrameMailbox::Publish(RasterPublication p) {
  if (!p || p.result->binding != binding_ || p.scene->binding != binding_ ||
      !p.result->config.IsValid() || p.result->config != p.scene->config ||
      p.result->layout_version != p.scene->layout_version ||
      p.result->segment_plan_version != p.scene->segment_plan_version ||
      p.result->outputs.empty() ||
      p.result->outputs.size() != p.scene->ordered_segments.size())
    return PublishResult::kRejected;
  for (const auto& entry : p.result->outputs)
    if (!entry.texture_ref || !entry.texture_ref->ready() ||
        entry.texture_ref->ready()->Poll() != CompletionStatus::kComplete)
      return PublishResult::kRejected;
  RasterPublication previous;
  PublishResult result;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_ || p.result->render_version <= last_render_ ||
        p.result->layout_version < last_layout_)
      return PublishResult::kRejected;
    last_render_ = p.result->render_version;
    last_layout_ = p.result->layout_version;
    previous = std::move(latest_);
    latest_ = std::move(p);
    result = wake_pending_ ? PublishResult::kCoalesced : PublishResult::kWake;
    wake_pending_ = true;
  }
  return result;
}
bool FrameMailbox::ReportFailure(PipelineError error) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (closed_) return false;
  error_ = error;
  bool wake = !wake_pending_;
  wake_pending_ = true;
  return wake;
}
PipelineError FrameMailbox::TakeFailure() {
  std::lock_guard<std::mutex> lock(mutex_);
  return std::exchange(error_, PipelineError::kNone);
}
RasterPublication FrameMailbox::TakeLatest() {
  std::lock_guard<std::mutex> lock(mutex_);
  wake_pending_ = false;
  return std::exchange(latest_, {});
}
void FrameMailbox::Close() {
  RasterPublication discarded;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    wake_pending_ = false;
    discarded = std::move(latest_);
  }
}
bool FrameMailbox::IsClosed() {
  std::lock_guard<std::mutex> lock(mutex_);
  return closed_;
}
}  // namespace clay::display
