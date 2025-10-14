// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_HELPER_SKITY_H_
#define CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_HELPER_SKITY_H_

#include "skity/render/canvas.hpp"

namespace clay {

class ComplexityCalculatorHelperSkity : public skity::Canvas {
 public:
  explicit ComplexityCalculatorHelperSkity(unsigned int ceiling)
      : is_complex_(false), ceiling_(ceiling), complexity_score_(0) {}
  using ClipOp = skity::Canvas::ClipOp;
  void OnClipRect(skity::Rect const& rect, ClipOp op) override {}
  void OnClipPath(skity::Path const& path, ClipOp op) override {}

  void OnDrawGlyphs(uint32_t count, const skity::GlyphID glyphs[],
                    const float position_x[], const float position_y[],
                    const skity::Font& font,
                    const skity::Paint& paint) override {}

  void OnDrawPaint(skity::Paint const& paint) override {
    if (IsComplex()) {
      return;
    }
    // Placeholder value here. This can be cheap (e.g. effectively a drawColor),
    // or expensive (e.g. a bitmap shader with an image filter)
    AccumulateComplexity(50);
  }

  void OnSave() override {}
  void OnRestore() override {}
  void OnRestoreToCount(int saveCount) override {}
  void OnFlush() override {}
  uint32_t OnGetWidth() const override { return 0; }
  uint32_t OnGetHeight() const override { return 0; };
  void OnUpdateViewport(uint32_t width, uint32_t height) override{};

  // This method finalizes the complexity score calculation and returns it
  unsigned int ComplexityScore() {
    // We hit our ceiling, so return that
    if (IsComplex()) {
      return Ceiling();
    }

    // Calculate the impact of any draw ops where the complexity is dependent
    // on the number of calls made.
    unsigned int batched_complexity = BatchedComplexity();

    // Check for overflow
    if (Ceiling() - complexity_score_ < batched_complexity) {
      return Ceiling();
    }

    return complexity_score_ + batched_complexity;
  }

 protected:
  void AccumulateComplexity(unsigned int complexity) {
    // Check to see if we will overflow by accumulating this complexity score
    if (ceiling_ - complexity_score_ < complexity) {
      is_complex_ = true;
      return;
    }

    complexity_score_ += complexity;
  }

  unsigned int CalculatePathComplexity(const skity::Path& path,
                                       unsigned int line_verb_cost,
                                       unsigned int quad_verb_cost,
                                       unsigned int conic_verb_cost,
                                       unsigned int cubic_verb_cost) {
    int verb_count = path.CountVerbs();
    unsigned int complexity = 0;
    for (int i = 0; i < verb_count; i++) {
      switch (path.GetVerb(i)) {
        case skity::Path::Verb::kLine:
          complexity += line_verb_cost;
          break;
        case skity::Path::Verb::kQuad:
          complexity += quad_verb_cost;
          break;
        case skity::Path::Verb::kConic:
          complexity += conic_verb_cost;
          break;
        case skity::Path::Verb::kCubic:
          complexity += cubic_verb_cost;
          break;
        default:
          break;
      }
    }
    return complexity;
  }

  // This calculates and returns the cost of draw calls which are batched and
  // thus have a time cost proportional to the number of draw calls made, such
  // as saveLayer and drawTextBlob.
  virtual unsigned int BatchedComplexity() = 0;

  inline bool IsAntiAliased(const skity::Paint& paint) {
    return paint.IsAntiAlias();
  }
  inline bool IsHairline(const skity::Paint& paint) {
    return paint.GetStrokeWidth() == 0.0f;
  }
  inline skity::Paint::Style Style(const skity::Paint& paint) {
    return paint.GetStyle();
  }

  inline bool IsComplex() { return is_complex_; }
  inline unsigned int Ceiling() { return ceiling_; }
  inline unsigned int CurrentComplexityScore() { return complexity_score_; }

 private:
  // If we exceed the ceiling (defaults to the largest number representable
  // by unsigned int), then set the is_complex_ bool and we no longer
  // accumulate.
  bool is_complex_;
  unsigned int ceiling_;

  unsigned int complexity_score_;
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_PICTURE_COMPLEXITY_HELPER_SKITY_H_
