// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/mock_drawable_image.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(MockDrawableImageTest, Callbacks) {
  auto drawable_image = std::make_shared<MockDrawableImage>();

  ASSERT_FALSE(drawable_image->gr_context_created());
  drawable_image->OnGrContextCreated();
  ASSERT_TRUE(drawable_image->gr_context_created());

  ASSERT_FALSE(drawable_image->gr_context_destroyed());
  drawable_image->OnGrContextDestroyed();
  ASSERT_TRUE(drawable_image->gr_context_destroyed());

  ASSERT_FALSE(drawable_image->unregistered());
  drawable_image->OnDrawableImageUnregistered();
  ASSERT_TRUE(drawable_image->unregistered());
}

TEST(MockDrawableImageTest, PaintCalls) {
  SkCanvas canvas;
  const skity::Rect paint_bounds1 = skity::Rect::MakeWH(1.0f, 1.0f);
  const skity::Rect paint_bounds2 = skity::Rect::MakeWH(2.0f, 2.0f);
  const SkSamplingOptions sampling;
  const auto expected_paint_calls = std::vector{
      MockDrawableImage::PaintCall{canvas, paint_bounds1, false, nullptr,
                                   sampling, nullptr,
                                   DrawableImage::FitMode::kScaleToFill},
      MockDrawableImage::PaintCall{canvas, paint_bounds2, true, nullptr,
                                   sampling, nullptr,
                                   DrawableImage::FitMode::kScaleToFill}};
  auto drawable_image = std::make_shared<MockDrawableImage>();
  DrawableImage::PaintContext context{
      .canvas = &canvas,
  };
  drawable_image->Paint(context, paint_bounds1, false, sampling,
                        DrawableImage::FitMode::kScaleToFill);
  drawable_image->Paint(context, paint_bounds2, true, sampling,
                        DrawableImage::FitMode::kScaleToFill);
  EXPECT_EQ(drawable_image->paint_calls(), expected_paint_calls);
}

TEST(MockDrawableImageTest, PaintCallsWithLinearSampling) {
  SkCanvas canvas;
  const skity::Rect paint_bounds1 = skity::Rect::MakeWH(1.0f, 1.0f);
  const skity::Rect paint_bounds2 = skity::Rect::MakeWH(2.0f, 2.0f);
  const auto sampling = SkSamplingOptions(SkFilterMode::kLinear);
  const auto expected_paint_calls = std::vector{
      MockDrawableImage::PaintCall{canvas, paint_bounds1, false, nullptr,
                                   sampling, nullptr,
                                   DrawableImage::FitMode::kScaleToFill},
      MockDrawableImage::PaintCall{canvas, paint_bounds2, true, nullptr,
                                   sampling, nullptr,
                                   DrawableImage::FitMode::kScaleToFill}};
  auto drawable_image = std::make_shared<MockDrawableImage>();
  DrawableImage::PaintContext context{
      .canvas = &canvas,
  };
  drawable_image->Paint(context, paint_bounds1, false, sampling,
                        DrawableImage::FitMode::kScaleToFill);
  drawable_image->Paint(context, paint_bounds2, true, sampling,
                        DrawableImage::FitMode::kScaleToFill);
  EXPECT_EQ(drawable_image->paint_calls(), expected_paint_calls);
}

}  // namespace testing
}  // namespace clay
