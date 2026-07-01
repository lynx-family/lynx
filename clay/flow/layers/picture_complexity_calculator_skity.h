// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_CALCULATOR_SKITY_H_
#define CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_CALCULATOR_SKITY_H_

#include <limits>
#include <memory>

#include "clay/flow/layers/picture_complexity.h"

namespace clay {

class PictureComplexityCalculatorSkity : public PictureComplexityCalculator {
 public:
  static PictureComplexityCalculatorSkity* GetInstance();

  void SetComplexityCeiling(unsigned int ceiling) override {}

  class SkityReplayHelper : public skity::Canvas {
   public:
    SkityReplayHelper() = default;
    ~SkityReplayHelper() override = default;
    using ClipOp = skity::Canvas::ClipOp;
    void OnClipRect(skity::Rect const& rect, ClipOp op) override {}
    void OnClipPath(skity::Path const& path, ClipOp op) override {}

    void OnSave() override {}
    void OnRestore() override {}
    void OnRestoreToCount(int saveCount) override {}
    void OnFlush() override {}
    uint32_t OnGetWidth() const override { return 0; }
    uint32_t OnGetHeight() const override { return 0; };

    void OnDrawGlyphs(uint32_t count, const skity::GlyphID glyphs[],
                      const float position_x[], const float position_y[],
                      const skity::Font& font,
                      const skity::Paint& paint) override;

    void OnDrawPaint(skity::Paint const& paint) override;

    void OnDrawLine(float x0, float y0, float x1, float y1,
                    skity::Paint const& paint) override;
    void OnDrawCircle(float cx, float cy, float radius,
                      skity::Paint const& paint) override;
    void OnDrawOval(skity::Rect const& oval,
                    skity::Paint const& paint) override;
    void OnDrawRect(skity::Rect const& rect,
                    skity::Paint const& paint) override;
    void OnDrawRRect(skity::RRect const& rrect,
                     skity::Paint const& paint) override;
    void OnDrawPath(skity::Path const& path,
                    skity::Paint const& paint) override;
    void OnSaveLayer(const skity::Rect& bounds,
                     const skity::Paint& paint) override;
    void OnDrawBlob(const skity::TextBlob* blob, float x, float y,
                    skity::Paint const& paint) override;
    void OnDrawImageRect(std::shared_ptr<skity::Image> image,
                         const skity::Rect& src, const skity::Rect& dst,
                         const skity::SamplingOptions& sampling,
                         skity::Paint const* paint) override;

    unsigned int ComplexityScore() const;

   private:
    bool HasPaintFilter(const skity::Paint& paint) const;
    void CheckPaintFilter(const skity::Paint& paint);
    unsigned int SaveLayerComplexityScore() const;

    bool has_filter_ = false;
    unsigned int save_layer_count_ = 0u;
  };

  unsigned int Compute(skity::DisplayList* display_list) override {
    if (!display_list->HasColorFilter() && !display_list->HasMaskFilter() &&
        !display_list->HasImageFilter() && !display_list->HasSaveLayer()) {
      return 0u;
    }

    SkityReplayHelper helper;
    display_list->Draw(&helper);
    return helper.ComplexityScore();
  }

  bool ShouldBeCached(unsigned int complexity_score) override {
    return complexity_score >= kFilterCacheScore;
  }

 private:
  static constexpr unsigned int kFilterCacheScore = 200u;
  static constexpr unsigned int kSaveLayerComplexityScore = 100u;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_CALCULATOR_SKITY_H_
