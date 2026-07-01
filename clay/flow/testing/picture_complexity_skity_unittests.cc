// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <limits>
#include <memory>
#include <vector>

#include "clay/flow/layers/picture_complexity_calculator_skity.h"
#include "clay/flow/layers/picture_complexity_gl.h"
#include "clay/flow/layers/picture_complexity_metal.h"
#include "clay/testing/testing.h"
#include "skity/effect/color_filter.hpp"
#include "skity/effect/image_filter.hpp"
#include "skity/effect/mask_filter.hpp"
#include "skity/effect/path_effect.hpp"
#include "skity/effect/shader.hpp"
#include "skity/recorder/picture_recorder.hpp"
#include "skity/text/font.hpp"

namespace clay {
namespace testing {
namespace {

template <typename DrawCallback>
std::unique_ptr<skity::DisplayList> MakeDisplayList(
    DrawCallback draw_callback) {
  skity::PictureRecorder recorder;
  recorder.BeginRecording(skity::Rect::MakeLTRB(0, 0, 100, 100));
  auto* canvas = recorder.GetRecordingCanvas();
  draw_callback(canvas);
  return recorder.FinishRecording();
}

std::unique_ptr<skity::DisplayList> MakeRectDisplayList(
    const skity::Paint& paint, int draw_count = 1) {
  return MakeDisplayList([&paint, draw_count](skity::Canvas* canvas) {
    auto rect = skity::Rect::MakeWH(10, 10);
    for (int i = 0; i < draw_count; i++) {
      canvas->DrawRect(rect, paint);
    }
  });
}

std::unique_ptr<skity::DisplayList> MakeSaveLayerDisplayList(
    const skity::Paint& paint, int save_layer_count) {
  return MakeDisplayList([&paint, save_layer_count](skity::Canvas* canvas) {
    auto bounds = skity::Rect::MakeWH(10, 10);
    for (int i = 0; i < save_layer_count; i++) {
      canvas->SaveLayer(bounds, paint);
      canvas->Restore();
    }
  });
}

std::vector<PictureComplexityCalculator*> Calculators() {
  return {PictureComplexityCalculatorSkity::GetInstance(),
          PictureNaiveComplexityCalculator::GetInstance(),
          PictureGLComplexityCalculator::GetInstance(),
          PictureMetalComplexityCalculator::GetInstance()};
}

std::vector<PictureComplexityCalculator*> ReplayCalculators() {
  return {PictureComplexityCalculatorSkity::GetInstance(),
          PictureGLComplexityCalculator::GetInstance(),
          PictureMetalComplexityCalculator::GetInstance()};
}

void ResetCeilings() {
  for (auto* calculator : Calculators()) {
    calculator->SetComplexityCeiling(std::numeric_limits<unsigned int>::max());
  }
}

void ExpectCachedDisplayList(skity::DisplayList* display_list) {
  ResetCeilings();
  for (auto* calculator : Calculators()) {
    unsigned int complexity_score = calculator->Compute(display_list);
    ASSERT_GT(complexity_score, 0u);
    ASSERT_TRUE(calculator->ShouldBeCached(complexity_score));
  }
}

void ExpectCachedWithFilterPaint(const skity::Paint& paint) {
  auto display_list = MakeRectDisplayList(paint);
  ExpectCachedDisplayList(display_list.get());
}

}  // namespace

TEST(PictureComplexitySkity, DrawOpsWithoutFiltersDoNotCache) {
  ResetCeilings();
  skity::Paint paint;
  auto display_list = MakeRectDisplayList(paint, 300);

  for (auto* calculator : Calculators()) {
    unsigned int complexity_score = calculator->Compute(display_list.get());
    ASSERT_EQ(complexity_score, 0u);
    ASSERT_FALSE(calculator->ShouldBeCached(complexity_score));
  }
}

TEST(PictureComplexitySkity, ShaderAndPathEffectDoNotCache) {
  ResetCeilings();

  skity::Paint shader_paint;
  skity::Point points[] = {skity::Point(0.f, 0.f, 0.f, 1.f),
                           skity::Point(100.f, 100.f, 0.f, 1.f)};
  skity::Vec4 colors[] = {skity::Vec4(1.f, 0.f, 0.f, 1.f),
                          skity::Vec4(0.f, 0.f, 1.f, 1.f)};
  shader_paint.SetShader(skity::Shader::MakeLinear(points, colors, nullptr, 2));
  auto shader_display_list = MakeRectDisplayList(shader_paint);
  for (auto* calculator : Calculators()) {
    unsigned int shader_score = calculator->Compute(shader_display_list.get());
    ASSERT_EQ(shader_score, 0u);
    ASSERT_FALSE(calculator->ShouldBeCached(shader_score));
  }

  skity::Paint path_effect_paint;
  float intervals[] = {1.f, 1.f};
  path_effect_paint.SetPathEffect(
      skity::PathEffect::MakeDashPathEffect(intervals, 2, 0.f));
  auto path_effect_display_list = MakeRectDisplayList(path_effect_paint);
  for (auto* calculator : Calculators()) {
    unsigned int path_effect_score =
        calculator->Compute(path_effect_display_list.get());
    ASSERT_EQ(path_effect_score, 0u);
    ASSERT_FALSE(calculator->ShouldBeCached(path_effect_score));
  }
}

TEST(PictureComplexitySkity, FilterPaintComputesCacheScore) {
  skity::Paint color_filter_paint;
  color_filter_paint.SetColorFilter(
      skity::ColorFilters::Blend(skity::Color_RED, skity::BlendMode::kSrc));
  ExpectCachedWithFilterPaint(color_filter_paint);

  skity::Paint mask_filter_paint;
  mask_filter_paint.SetMaskFilter(
      skity::MaskFilter::MakeBlur(skity::BlurStyle::kNormal, 10.f));
  ExpectCachedWithFilterPaint(mask_filter_paint);

  skity::Paint image_filter_paint;
  image_filter_paint.SetImageFilter(skity::ImageFilters::Blur(10.f, 10.f));
  ExpectCachedWithFilterPaint(image_filter_paint);
}

TEST(PictureComplexitySkity, DrawPaintWithFilterComputesCacheScore) {
  skity::Paint paint;
  paint.SetImageFilter(skity::ImageFilters::Blur(10.f, 10.f));
  auto display_list = MakeDisplayList(
      [&paint](skity::Canvas* canvas) { canvas->DrawPaint(paint); });

  ExpectCachedDisplayList(display_list.get());
}

TEST(PictureComplexitySkity, DrawGlyphsWithFilterComputesCacheScore) {
  skity::Paint paint;
  paint.SetColorFilter(
      skity::ColorFilters::Blend(skity::Color_RED, skity::BlendMode::kSrc));
  auto display_list = MakeDisplayList([&paint](skity::Canvas* canvas) {
    skity::GlyphID glyphs[] = {1};
    float position_x[] = {0.f};
    float position_y[] = {0.f};
    skity::Font font;
    canvas->DrawGlyphs(1, glyphs, position_x, position_y, font, paint);
  });

  ExpectCachedDisplayList(display_list.get());
}

TEST(PictureComplexitySkity, SaveLayerWithFilterComputesCacheScore) {
  skity::Paint paint;
  paint.SetMaskFilter(
      skity::MaskFilter::MakeBlur(skity::BlurStyle::kNormal, 10.f));
  auto display_list = MakeSaveLayerDisplayList(paint, 1);

  ExpectCachedDisplayList(display_list.get());
}

TEST(PictureComplexitySkity, SingleSaveLayerWithoutFilterStaysBelowCacheScore) {
  ResetCeilings();
  skity::Paint paint;
  auto display_list = MakeSaveLayerDisplayList(paint, 1);

  for (auto* calculator : ReplayCalculators()) {
    unsigned int complexity_score = calculator->Compute(display_list.get());
    ASSERT_GT(complexity_score, 0u);
    ASSERT_FALSE(calculator->ShouldBeCached(complexity_score));
  }
}

TEST(PictureComplexitySkity, SaveLayersWithoutFilterComputeCacheScore) {
  skity::Paint paint;
  auto display_list = MakeSaveLayerDisplayList(paint, 2);

  ExpectCachedDisplayList(display_list.get());
}

}  // namespace testing
}  // namespace clay
