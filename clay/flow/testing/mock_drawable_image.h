// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_FLOW_TESTING_MOCK_DRAWABLE_IMAGE_H_
#define CLAY_FLOW_TESTING_MOCK_DRAWABLE_IMAGE_H_

#include <ostream>
#include <vector>

#include "clay/common/graphics/drawable_image.h"
#include "clay/testing/assertions_skia.h"

namespace clay {
namespace testing {

// Mock implementation of the |DrawableImage| interface that does not interact
// with any actual rendering backend (such as GPU or CPU). It simply records the
// list of various method calls made so that tests can later verify them against
// expected data.
class MockDrawableImage : public DrawableImage {
 public:
  struct PaintCall {
    SkCanvas& canvas;
    skity::Rect bounds;
    bool freeze;
    GrDirectContext* context;
    SkSamplingOptions sampling;
    const SkPaint* paint;
    FitMode fit_mode;
  };

  MockDrawableImage() = default;

  ImageType GetType() const override { return ImageType::kMock; }

  void SetFrameAvailableCallback(const fml::closure& callback) override {}

  // Called from raster thread.
  void Paint(PaintContext& context, const skity::Rect& bounds, bool freeze,
             const SkSamplingOptions& sampling, FitMode fit_mode) override;

  void OnGrContextCreated() override { gr_context_created_ = true; }
  void OnGrContextDestroyed() override { gr_context_destroyed_ = true; }
  void MarkNewFrameAvailable() override {}
  void OnDrawableImageUnregistered() override { unregistered_ = true; }

  const std::vector<PaintCall>& paint_calls() { return paint_calls_; }
  bool gr_context_created() { return gr_context_created_; }
  bool gr_context_destroyed() { return gr_context_destroyed_; }
  bool unregistered() { return unregistered_; }

 private:
  std::vector<PaintCall> paint_calls_;
  bool gr_context_created_ = false;
  bool gr_context_destroyed_ = false;
  bool unregistered_ = false;
};

extern bool operator==(const MockDrawableImage::PaintCall& a,
                       const MockDrawableImage::PaintCall& b);
extern std::ostream& operator<<(std::ostream& os,
                                const MockDrawableImage::PaintCall& data);

}  // namespace testing
}  // namespace clay

#endif  // CLAY_FLOW_TESTING_MOCK_DRAWABLE_IMAGE_H_
