// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_SEGMENT_RECORDER_H_
#define CLAY_FLOW_COMPOSITOR_SEGMENT_RECORDER_H_
#include <memory>
#include <vector>

#include "clay/flow/layers/drawable_image_layer.h"
#include "skity/recorder/picture_recorder.hpp"
namespace clay::display {
class SegmentPlanner {
 public:
  // Conservative whole-page fallback for grouped effects or more than 8
  // outputs.
  static bool CanSplit(const Layer& root);
};
struct RecordedSegment {
  int64_t image_id = 0;
  bool dynamic = false, frozen = false;
  skity::Rect bounds;
  std::shared_ptr<skity::DisplayList> recording;
  DrawableImageLayer::SegmentPaint repaint;
};
class SegmentRecorder {
 public:
  explicit SegmentRecorder(skity::Vec2 size);
  GrCanvas* canvas() const { return recorder_->GetRecordingCanvas(); }
  void RecordDynamic(const DrawableImageLayer& layer, PaintContext& context);
  std::vector<RecordedSegment> Finish();

 private:
  void Begin();
  void FinishStatic();
  skity::Vec2 size_;
  std::unique_ptr<skity::PictureRecorder> recorder_;
  std::vector<RecordedSegment> segments_;
};
}  // namespace clay::display
#endif  // CLAY_FLOW_COMPOSITOR_SEGMENT_RECORDER_H_
