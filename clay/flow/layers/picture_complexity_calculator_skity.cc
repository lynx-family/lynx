// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/picture_complexity_calculator_skity.h"

namespace clay {

using SkityReplayHelper = PictureComplexityCalculatorSkity::SkityReplayHelper;

PictureComplexityCalculatorSkity*
PictureComplexityCalculatorSkity::GetInstance() {
  static PictureComplexityCalculatorSkity instance;
  return &instance;
}

bool SkityReplayHelper::IsPaintComplex(const skity::Paint& paint) const {
  return paint.GetColorFilter() != nullptr ||
         paint.GetMaskFilter() != nullptr ||
         paint.GetImageFilter() != nullptr || paint.GetShader() != nullptr ||
         paint.GetPathEffect() != nullptr;
}

void SkityReplayHelper::CheckPaint(const skity::Paint& paint) {
  if (!is_complex_ && IsPaintComplex(paint)) {
    is_complex_ = true;
  }
}

void SkityReplayHelper::OnDrawGlyphs(uint32_t count,
                                     const skity::GlyphID glyphs[],
                                     const float position_x[],
                                     const float position_y[],
                                     const skity::Font& font,
                                     const skity::Paint& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawPaint(skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawLine(float x0, float y0, float x1, float y1,
                                   skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawCircle(float cx, float cy, float radius,
                                     skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawOval(skity::Rect const& oval,
                                   skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawRect(skity::Rect const& rect,
                                   skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawRRect(skity::RRect const& rrect,
                                    skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawPath(skity::Path const& path,
                                   skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnSaveLayer(const skity::Rect& bounds,
                                    const skity::Paint& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawBlob(const skity::TextBlob* blob, float x,
                                   float y, skity::Paint const& paint) {
  CheckPaint(paint);
}

void SkityReplayHelper::OnDrawImageRect(std::shared_ptr<skity::Image> image,
                                        const skity::Rect& src,
                                        const skity::Rect& dst,
                                        const skity::SamplingOptions& sampling,
                                        skity::Paint const* paint) {
  if (paint) {
    CheckPaint(*paint);
  }
}

}  // namespace clay
