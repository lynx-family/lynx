// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/compositor/segment_recorder.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/picture_layer.h"
namespace clay::display {
namespace {
skity::Rect PixelBounds(skity::Rect bounds, skity::Vec2 viewport) {
  auto full = skity::Rect::MakeSize(viewport);
  if (!bounds.IsFinite()) return full;
  bounds.Outset(2);
  bounds.RoundOut();
  if (!bounds.Intersect(full)) return skity::Rect::MakeWH(1, 1);
  return bounds;
}
bool Visit(const Layer& layer, size_t& external_image_count) {
  if (!layer.CanSegmentDisplay() || layer.HasUnstableDisplayContent())
    return false;
  if (auto* picture = layer.as_picture_layer()) {
    auto recording = picture->picture();
    if (!recording) return false;
    auto ops = recording->Search(recording->GetBounds());
    // Unknown/raw display lists without an operation index are conservative.
    if (ops.empty() && recording->OpCount()) return false;
    for (auto offset : ops) {
      auto* paint = recording->GetOpPaintByOffset(offset);
      if (!paint || paint->GetBlendMode() != skity::BlendMode::kSrcOver)
        return false;
    }
  }
  if (layer.as_drawable_image_layer() && ++external_image_count > 3)
    return false;
  if (auto* group = layer.as_container_layer()) {
    for (const auto& child : group->layers())
      if (!Visit(*child, external_image_count)) return false;
  }
  return true;
}
}  // namespace
bool SegmentPlanner::CanSplit(const Layer& root) {
  TRACE_EVENT("clay", "SegmentPlanner::CanSplit");
  size_t external_image_count = 0;
  return Visit(root, external_image_count) && external_image_count > 0;
}
SegmentRecorder::SegmentRecorder(skity::Vec2 size) : size_(size) { Begin(); }
void SegmentRecorder::Begin() {
  recorder_ = std::make_unique<skity::PictureRecorder>();
  recorder_->BeginRecording(skity::Rect::MakeSize(size_));
}
void SegmentRecorder::FinishStatic() {
  TRACE_EVENT("clay", "SegmentRecorder::FinishStatic");
  RecordedSegment segment;
  segment.recording = recorder_->FinishRecording();
  auto& recording = *segment.recording;
  segment.bounds = (recording.HasSaveLayer() || recording.HasImageFilter() ||
                    recording.HasMaskFilter())
                       ? skity::Rect::MakeSize(size_)
                       : PixelBounds(recording.GetBounds(), size_);
  segments_.push_back(std::move(segment));
}
void SegmentRecorder::RecordDynamic(const DrawableImageLayer& layer,
                                    PaintContext& context) {
  TRACE_EVENT("clay", "SegmentRecorder::RecordDynamic");
  auto repaint = layer.CaptureSegmentPaint(context);
  auto bounds = PixelBounds(
      context.state_stack.transform_4x4().MapRect(layer.paint_bounds()), size_);
  context.state_stack.set_delegate(static_cast<GrCanvas*>(nullptr));
  FinishStatic();
  RecordedSegment segment;
  segment.dynamic = true;
  segment.bounds = bounds;
  segment.image_id = layer.image_id();
  segment.frozen = layer.frozen();
  segment.repaint = std::move(repaint);
  segments_.push_back(std::move(segment));
  Begin();
  context.canvas = canvas();
  context.state_stack.set_delegate(context.canvas);
}
std::vector<RecordedSegment> SegmentRecorder::Finish() {
  FinishStatic();
  return std::move(segments_);
}
}  // namespace clay::display
