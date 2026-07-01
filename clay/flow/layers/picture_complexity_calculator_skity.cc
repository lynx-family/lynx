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

bool SkityReplayHelper::HasPaintFilter(const skity::Paint& paint) const {
  return paint.GetColorFilter() != nullptr ||
         paint.GetMaskFilter() != nullptr || paint.GetImageFilter() != nullptr;
}

void SkityReplayHelper::CheckPaintFilter(const skity::Paint& paint) {
  if (!has_filter_ && HasPaintFilter(paint)) {
    has_filter_ = true;
  }
}

unsigned int SkityReplayHelper::SaveLayerComplexityScore() const {
  if (save_layer_count_ >
      std::numeric_limits<unsigned int>::max() /
          PictureComplexityCalculatorSkity::kSaveLayerComplexityScore) {
    return std::numeric_limits<unsigned int>::max();
  }
  return save_layer_count_ *
         PictureComplexityCalculatorSkity::kSaveLayerComplexityScore;
}

unsigned int SkityReplayHelper::ComplexityScore() const {
  if (has_filter_) {
    return PictureComplexityCalculatorSkity::kFilterCacheScore;
  }
  return SaveLayerComplexityScore();
}

void SkityReplayHelper::OnDrawGlyphs(uint32_t count,
                                     const skity::GlyphID glyphs[],
                                     const float position_x[],
                                     const float position_y[],
                                     const skity::Font& font,
                                     const skity::Paint& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawPaint(skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawLine(float x0, float y0, float x1, float y1,
                                   skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawCircle(float cx, float cy, float radius,
                                     skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawOval(skity::Rect const& oval,
                                   skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawRect(skity::Rect const& rect,
                                   skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawRRect(skity::RRect const& rrect,
                                    skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawPath(skity::Path const& path,
                                   skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnSaveLayer(const skity::Rect& bounds,
                                    const skity::Paint& paint) {
  CheckPaintFilter(paint);
  save_layer_count_++;
}

void SkityReplayHelper::OnDrawBlob(const skity::TextBlob* blob, float x,
                                   float y, skity::Paint const& paint) {
  CheckPaintFilter(paint);
}

void SkityReplayHelper::OnDrawImageRect(std::shared_ptr<skity::Image> image,
                                        const skity::Rect& src,
                                        const skity::Rect& dst,
                                        const skity::SamplingOptions& sampling,
                                        skity::Paint const* paint) {
  if (paint) {
    CheckPaintFilter(*paint);
  }
}

}  // namespace clay
