// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display/frame_mailbox.h"

#include <unordered_set>
#include <utility>
namespace clay::display {
FrameMailbox::PublishResult FrameMailbox::Publish(
    const PageSceneSnapshot& scene, std::shared_ptr<const RasterResult> p) {
  if (!p || p->binding != binding_ || scene.binding != binding_ ||
      !p->config.IsValid() || p->config != scene.config || !p->layout_version ||
      p->layout_version != scene.layout_version || !p->segment_plan_version ||
      p->segment_plan_version != scene.segment_plan_version ||
      p->outputs.empty() || p->outputs.size() > 8 ||
      p->outputs.size() != scene.ordered_segments.size())
    return PublishResult::kRejected;
  std::unordered_set<SegmentId> ids;
  for (size_t i = 0; i < p->outputs.size(); ++i) {
    const auto& entry = p->outputs[i];
    const auto& placement = scene.ordered_segments[i];
    if (!entry.texture_ref || !entry.texture_ref->ready() ||
        !entry.segment_id || !ids.insert(entry.segment_id).second ||
        entry.segment_id != placement.segment_id ||
        entry.content_version != entry.texture_ref->content_version() ||
        entry.properties.x != placement.properties.x ||
        entry.properties.y != placement.properties.y)
      return PublishResult::kRejected;
    const auto& key = entry.texture_ref->resource()->key();
    const auto& image = entry.texture_ref->image();
    if (key.device_id != p->config.device_id ||
        key.device_generation != p->config.device_generation ||
        image.format != p->config.format || image.alpha != p->config.alpha ||
        image.color != p->config.color || !image.width || !image.height ||
        !key.resource_id || !key.allocation_generation ||
        image.valid_rect.x < 0 || image.valid_rect.y < 0 ||
        !image.valid_rect.width || !image.valid_rect.height ||
        static_cast<uint64_t>(image.valid_rect.x) + image.valid_rect.width >
            image.width ||
        static_cast<uint64_t>(image.valid_rect.y) + image.valid_rect.height >
            image.height)
      return PublishResult::kRejected;
  }
  std::shared_ptr<const RasterResult> previous;
  PublishResult result;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_ || p->render_version <= last_render_ ||
        p->layout_version < last_layout_)
      return PublishResult::kRejected;
    last_render_ = p->render_version;
    last_layout_ = p->layout_version;
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
std::shared_ptr<const RasterResult> FrameMailbox::TakeLatest() {
  std::lock_guard<std::mutex> lock(mutex_);
  wake_pending_ = false;
  return std::exchange(latest_, {});
}
void FrameMailbox::Close() {
  std::shared_ptr<const RasterResult> discarded;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    wake_pending_ = false;
    discarded = std::move(latest_);
  }
}
}  // namespace clay::display
