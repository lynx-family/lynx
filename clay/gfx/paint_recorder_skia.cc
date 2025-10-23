// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "clay/gfx/paint_recorder.h"
#include "clay/gfx/skia/skia_canvas.h"

namespace clay {

GraphicsCanvas* PaintRecorder::BeginRecording(const skity::Rect& bounds) {
  skity::Rect rect = skity::Rect::MakeLTRB(bounds.Left(), bounds.Top(),
                                           bounds.Right(), bounds.Bottom());
  canvas_ = std::make_unique<SkiaRecorderCanvas>(rect, unref_queue_);
  return canvas_.get();
}

GraphicsCanvas* PaintRecorder::BeginRecording(const SkBitmap& bitmap) {
  canvas_ = std::make_unique<SkiaBitmapCanvas>(bitmap);
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
