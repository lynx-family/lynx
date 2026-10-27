// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/paint_recorder.h"
#include "clay/gfx/skity/skity_canvas.h"

namespace clay {
GraphicsCanvas* PaintRecorder::BeginRecording(const skity::Rect& bounds) {
  canvas_ = std::make_unique<SkityRecorderCanvas>(bounds, unref_queue_);
  return canvas_.get();
}

bool PaintRecorder::IsRecording() const { return canvas_.get() != nullptr; }

std::unique_ptr<Picture> PaintRecorder::FinishRecordingAsPicture() {
  if (!canvas_.get()) {
    return nullptr;
  }
  auto result = canvas_->FinishRecordingAsPicture();
  canvas_.reset();
  return result;
}
}  // namespace clay
