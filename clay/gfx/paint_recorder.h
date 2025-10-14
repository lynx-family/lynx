// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_PAINT_RECORDER_H_
#define CLAY_GFX_PAINT_RECORDER_H_

#include <memory>
#include <utility>

#include "clay/gfx/gpu_object.h"
#include "clay/gfx/picture.h"
#include "skity/geometry/rect.hpp"

namespace clay {

class GraphicsCanvas;

class PaintRecorder {
 public:
  explicit PaintRecorder(fml::RefPtr<GPUUnrefQueue> unref_queue)
      : unref_queue_(unref_queue) {}

  PaintRecorder(const PaintRecorder&) = delete;
  ~PaintRecorder() = default;
  PaintRecorder& operator=(const PaintRecorder&) = delete;

  GraphicsCanvas* BeginRecording(const skity::Rect& bounds);
#ifndef ENABLE_SKITY
  GraphicsCanvas* BeginRecording(const SkBitmap& bitmap);
#endif  // ENABLE_SKITY
  GraphicsCanvas* BeginRecording(float width, float height) {
    return BeginRecording(skity::Rect::MakeWH(width, height));
  }
  bool IsRecording() const;

  std::unique_ptr<Picture> FinishRecordingAsPicture();

  GraphicsCanvas* Canvas() { return canvas_.get(); }

 private:
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
  std::unique_ptr<GraphicsCanvas> canvas_;
};

}  // namespace clay

#endif  // CLAY_GFX_PAINT_RECORDER_H_
