// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/layer_state_stack.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/gfx/testing_utils.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

#ifndef NDEBUG
TEST(LayerStateStack, AccessorsDieWithoutDelegate) {
  LayerStateStack state_stack;

  EXPECT_DEATH_IF_SUPPORTED(state_stack.device_cull_rect(),
                            "LayerStateStack state queried without a delegate");
  EXPECT_DEATH_IF_SUPPORTED(state_stack.local_cull_rect(),
                            "LayerStateStack state queried without a delegate");
  EXPECT_DEATH_IF_SUPPORTED(state_stack.transform_4x4(),
                            "LayerStateStack state queried without a delegate");
  EXPECT_DEATH_IF_SUPPORTED(state_stack.content_culled({}),
                            "LayerStateStack state queried without a delegate");
  {
    // state_stack.set_preroll_delegate(kGiantRect, skity::Matrix());
    auto mutator = state_stack.save();
    mutator.applyOpacity({}, 0.5);
    state_stack.clear_delegate();
    auto restore = state_stack.applyState({}, 0);
  }
}
#endif

TEST(LayerStateStack, Defaults) {
  LayerStateStack state_stack;

  ASSERT_EQ(state_stack.canvas_delegate(), nullptr);
  ASSERT_EQ(state_stack.checkerboard_func(), nullptr);
  ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);
  ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_bounds(), skity::Rect());

  state_stack.set_preroll_delegate(kGiantRect, skity::Matrix());
  ASSERT_EQ(state_stack.device_cull_rect(), kGiantRect);
  ASSERT_EQ(state_stack.local_cull_rect(), kGiantRect);
  ASSERT_EQ(state_stack.transform_4x4(), skity::Matrix());

  SkPaint sk_paint;
  state_stack.fill(sk_paint);
  ASSERT_EQ(sk_paint, SkPaint());

  DlPaint dl_paint;
  state_stack.fill(dl_paint);
  ASSERT_EQ(dl_paint, DlPaint());
}

TEST(LayerStateStack, SingularDelegate) {
  LayerStateStack state_stack;
  ASSERT_EQ(state_stack.canvas_delegate(), nullptr);

  MockCanvas canvas;

  // builder delegate -> canvas delegate
  state_stack.set_delegate(&canvas);
  ASSERT_EQ(state_stack.canvas_delegate(), &canvas);

  // builder delegate -> no delegate
  state_stack.clear_delegate();
  ASSERT_EQ(state_stack.canvas_delegate(), nullptr);

  // canvas delegate -> no delegate
  state_stack.set_delegate(&canvas);
  state_stack.clear_delegate();
  ASSERT_EQ(state_stack.canvas_delegate(), nullptr);
}

TEST(LayerStateStack, Opacity) {
  skity::Rect rect = {10, 10, 20, 20};

  LayerStateStack state_stack;
  state_stack.set_preroll_delegate(skity::Rect::MakeLTRB(0, 0, 50, 50));
  {
    auto mutator = state_stack.save();
    mutator.applyOpacity(rect, 0.5f);

    ASSERT_EQ(state_stack.outstanding_opacity(), 0.5f);
    ASSERT_EQ(state_stack.outstanding_bounds(), rect);

    // Check nested opacities multiply with each other
    {
      auto mutator2 = state_stack.save();
      mutator.applyOpacity(rect, 0.5f);

      ASSERT_EQ(state_stack.outstanding_opacity(), 0.25f);
      ASSERT_EQ(state_stack.outstanding_bounds(), rect);
    }

    ASSERT_EQ(state_stack.outstanding_opacity(), 0.5f);
    ASSERT_EQ(state_stack.outstanding_bounds(), rect);
  }

  ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);
  ASSERT_EQ(state_stack.outstanding_bounds(), skity::Rect());
}

TEST(LayerStateStack, ColorFilter) {
  skity::Rect rect = {10, 10, 20, 20};
  std::shared_ptr<DlBlendColorFilter> outer_filter =
      std::make_shared<DlBlendColorFilter>(DlColor::kYellow(),
                                           DlBlendMode::kColorBurn);
  std::shared_ptr<DlBlendColorFilter> inner_filter =
      std::make_shared<DlBlendColorFilter>(DlColor::kRed(),
                                           DlBlendMode::kColorBurn);

  LayerStateStack state_stack;
  state_stack.set_preroll_delegate(skity::Rect::MakeLTRB(0, 0, 50, 50));
  {
    auto mutator = state_stack.save();
    mutator.applyColorFilter(rect, outer_filter);

    ASSERT_EQ(state_stack.outstanding_color_filter(), outer_filter);

    // Check nested color filters result in nested saveLayers
    {
      auto mutator2 = state_stack.save();
      mutator.applyColorFilter(rect, inner_filter);

      ASSERT_EQ(state_stack.outstanding_color_filter(), inner_filter);
    }

    ASSERT_EQ(state_stack.outstanding_color_filter(), outer_filter);
  }

  ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);
}

TEST(LayerStateStack, ImageFilter) {
  skity::Rect rect = {10, 10, 20, 20};
  std::shared_ptr<DlBlurImageFilter> outer_filter =
      std::make_shared<DlBlurImageFilter>(2.0f, 2.0f, DlTileMode::kClamp);
  std::shared_ptr<DlBlurImageFilter> inner_filter =
      std::make_shared<DlBlurImageFilter>(3.0f, 3.0f, DlTileMode::kClamp);
  skity::Rect outer_src_rect_tmp;
  ASSERT_EQ(inner_filter->map_local_bounds(rect, outer_src_rect_tmp),
            &outer_src_rect_tmp);
  skity::Rect outer_src_rect = outer_src_rect_tmp;

  LayerStateStack state_stack;
  state_stack.set_preroll_delegate(skity::Rect::MakeLTRB(0, 0, 50, 50));
  {
    auto mutator = state_stack.save();
    mutator.applyImageFilter(outer_src_rect, outer_filter);

    ASSERT_EQ(state_stack.outstanding_image_filter(), outer_filter);

    // Check nested color filters result in nested saveLayers
    {
      auto mutator2 = state_stack.save();
      mutator.applyImageFilter(rect, inner_filter);

      ASSERT_EQ(state_stack.outstanding_image_filter(), inner_filter);
    }

    ASSERT_EQ(state_stack.outstanding_image_filter(), outer_filter);
  }

  ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
}

TEST(LayerStateStack, OpacityAndColorFilterInteraction) {
  skity::Rect rect = {10, 10, 20, 20};
  std::shared_ptr<DlBlendColorFilter> color_filter =
      std::make_shared<DlBlendColorFilter>(DlColor::kYellow(),
                                           DlBlendMode::kColorBurn);

  SkCanvas builder;
  LayerStateStack state_stack;
  state_stack.set_delegate(&builder);
  ASSERT_EQ(builder.getSaveCount(), 1);

  {
    auto mutator1 = state_stack.save();
    ASSERT_EQ(builder.getSaveCount(), 1);
    mutator1.applyOpacity(rect, 0.5f);
    ASSERT_EQ(builder.getSaveCount(), 1);

    {
      auto mutator2 = state_stack.save();
      ASSERT_EQ(builder.getSaveCount(), 1);
      mutator2.applyColorFilter(rect, color_filter);

      // The opacity will have been resolved by a saveLayer
      ASSERT_EQ(builder.getSaveCount(), 2);
      ASSERT_EQ(state_stack.outstanding_color_filter(), color_filter);
      ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);
    }
    ASSERT_EQ(builder.getSaveCount(), 1);
    ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);
    ASSERT_EQ(state_stack.outstanding_opacity(), 0.5f);
  }
  ASSERT_EQ(builder.getSaveCount(), 1);
  ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);

  {
    auto mutator1 = state_stack.save();
    ASSERT_EQ(builder.getSaveCount(), 1);
    mutator1.applyColorFilter(rect, color_filter);
    ASSERT_EQ(builder.getSaveCount(), 1);

    {
      auto mutator2 = state_stack.save();
      ASSERT_EQ(builder.getSaveCount(), 1);
      mutator2.applyOpacity(rect, 0.5f);

      // color filter applied to opacity can be applied together
      ASSERT_EQ(builder.getSaveCount(), 1);
      ASSERT_EQ(state_stack.outstanding_color_filter(), color_filter);
      ASSERT_EQ(state_stack.outstanding_opacity(), 0.5f);
    }
    ASSERT_EQ(builder.getSaveCount(), 1);
    ASSERT_EQ(state_stack.outstanding_color_filter(), color_filter);
    ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);
  }
  ASSERT_EQ(builder.getSaveCount(), 1);
  ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);
}

TEST(LayerStateStack, OpacityAndImageFilterInteraction) {
  skity::Rect rect = {10, 10, 20, 20};
  std::shared_ptr<DlBlurImageFilter> image_filter =
      std::make_shared<DlBlurImageFilter>(2.0f, 2.0f, DlTileMode::kClamp);

  SkCanvas builder;
  LayerStateStack state_stack;
  state_stack.set_delegate(&builder);
  ASSERT_EQ(builder.getSaveCount(), 1);

  {
    auto mutator1 = state_stack.save();
    ASSERT_EQ(builder.getSaveCount(), 1);
    mutator1.applyOpacity(rect, 0.5f);
    ASSERT_EQ(builder.getSaveCount(), 1);

    {
      auto mutator2 = state_stack.save();
      ASSERT_EQ(builder.getSaveCount(), 1);
      mutator2.applyImageFilter(rect, image_filter);

      // opacity applied to image filter can be applied together
      ASSERT_EQ(builder.getSaveCount(), 1);
      ASSERT_EQ(state_stack.outstanding_image_filter(), image_filter);
      ASSERT_EQ(state_stack.outstanding_opacity(), 0.5f);
    }
    ASSERT_EQ(builder.getSaveCount(), 1);
    ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
    ASSERT_EQ(state_stack.outstanding_opacity(), 0.5f);
  }
  ASSERT_EQ(builder.getSaveCount(), 1);
  ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);

  {
    auto mutator1 = state_stack.save();
    ASSERT_EQ(builder.getSaveCount(), 1);
    mutator1.applyImageFilter(rect, image_filter);
    ASSERT_EQ(builder.getSaveCount(), 1);

    {
      auto mutator2 = state_stack.save();
      ASSERT_EQ(builder.getSaveCount(), 1);
      mutator2.applyOpacity(rect, 0.5f);

      // The image filter will have been resolved by a saveLayer
      ASSERT_EQ(builder.getSaveCount(), 2);
      ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
      ASSERT_EQ(state_stack.outstanding_opacity(), 0.5f);
    }
    ASSERT_EQ(builder.getSaveCount(), 1);
    ASSERT_EQ(state_stack.outstanding_image_filter(), image_filter);
    ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);
  }
  ASSERT_EQ(builder.getSaveCount(), 1);
  ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_opacity(), 1.f);
}

TEST(LayerStateStack, ColorFilterAndImageFilterInteraction) {
  skity::Rect rect = {10, 10, 20, 20};
  std::shared_ptr<DlBlendColorFilter> color_filter =
      std::make_shared<DlBlendColorFilter>(DlColor::kYellow(),
                                           DlBlendMode::kColorBurn);
  std::shared_ptr<DlBlurImageFilter> image_filter =
      std::make_shared<DlBlurImageFilter>(2.0f, 2.0f, DlTileMode::kClamp);

  SkCanvas builder;
  LayerStateStack state_stack;
  state_stack.set_delegate(&builder);
  ASSERT_EQ(builder.getSaveCount(), 1);

  {
    auto mutator1 = state_stack.save();
    ASSERT_EQ(builder.getSaveCount(), 1);
    mutator1.applyColorFilter(rect, color_filter);
    ASSERT_EQ(builder.getSaveCount(), 1);

    {
      auto mutator2 = state_stack.save();
      ASSERT_EQ(builder.getSaveCount(), 1);
      mutator2.applyImageFilter(rect, image_filter);

      // color filter applied to image filter can be applied together
      ASSERT_EQ(builder.getSaveCount(), 1);
      ASSERT_EQ(state_stack.outstanding_image_filter(), image_filter);
      ASSERT_EQ(state_stack.outstanding_color_filter(), color_filter);
    }
    ASSERT_EQ(builder.getSaveCount(), 1);
    ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
    ASSERT_EQ(state_stack.outstanding_color_filter(), color_filter);
  }
  ASSERT_EQ(builder.getSaveCount(), 1);
  ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);

  {
    auto mutator1 = state_stack.save();
    ASSERT_EQ(builder.getSaveCount(), 1);
    mutator1.applyImageFilter(rect, image_filter);
    ASSERT_EQ(builder.getSaveCount(), 1);

    {
      auto mutator2 = state_stack.save();
      ASSERT_EQ(builder.getSaveCount(), 1);
      mutator2.applyColorFilter(rect, color_filter);

      // The image filter will have been resolved by a saveLayer
      ASSERT_EQ(builder.getSaveCount(), 2);
      ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
      ASSERT_EQ(state_stack.outstanding_color_filter(), color_filter);
    }
    ASSERT_EQ(builder.getSaveCount(), 1);
    ASSERT_EQ(state_stack.outstanding_image_filter(), image_filter);
    ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);
  }
  ASSERT_EQ(builder.getSaveCount(), 1);
  ASSERT_EQ(state_stack.outstanding_image_filter(), nullptr);
  ASSERT_EQ(state_stack.outstanding_color_filter(), nullptr);
}

}  // namespace testing
}  // namespace clay
