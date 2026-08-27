// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "clay/flow/layers/shader_mask_layer.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/rendering_backend.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {
namespace {

class SolidRectLayer final : public Layer {
 public:
  SolidRectLayer(const skity::Rect& bounds, GrColor color)
      : bounds_(bounds), color_(color) {}

  void Preroll(PrerollContext*) override { set_paint_bounds(bounds_); }

  void Paint(PaintContext& context) const override {
    GrPaint paint;
    PAINT_SET_COLOR(paint, color_);
    CANVAS_DRAW_RECT(context.canvas, bounds_, paint);
  }

 private:
  skity::Rect bounds_;
  GrColor color_;
};

TEST(ShaderMaskLayerPictureTest, PictureMaskCompositesChildPixels) {
  constexpr int kSurfaceSize = 64;
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(10.0f, 10.0f, 50.0f, 50.0f);

  GraphicsContext mask_context(nullptr);
  ASSERT_TRUE(mask_context.BeginRecording(skity::Rect::MakeWH(40.0f, 40.0f)));
  Paint mask_paint;
  mask_paint.setColor(Color::kWhite());
  mask_context.DrawRect(mask_paint,
                        skity::Rect::MakeLTRB(0.0f, 0.0f, 20.0f, 40.0f));
  auto mask_picture = mask_context.FinishRecording();
  ASSERT_NE(mask_picture, nullptr);

  auto layer = std::make_shared<ShaderMaskLayer>(
      child_bounds, BlendMode::kDstIn,
      std::shared_ptr<Picture>(std::move(mask_picture)));
  layer->Add(
      std::make_shared<SolidRectLayer>(child_bounds, Color::kRed().Value()));

  LayerStateStack preroll_state_stack;
  preroll_state_stack.set_preroll_delegate(kGiantRect, skity::Matrix());
  FixedRefreshRateStopwatch raster_time;
  FixedRefreshRateStopwatch ui_time;
  auto drawable_image_registry = std::make_shared<DrawableImageRegistry>();
  std::vector<RasterCacheItem*> raster_cached_entries;
  PrerollContext preroll_context{
      .raster_cache = nullptr,
      .gr_context = nullptr,
      .compositor_state = nullptr,
      .state_stack = preroll_state_stack,
      .surface_needs_readback = false,
      .raster_time = raster_time,
      .ui_time = ui_time,
      .drawable_image_registry = drawable_image_registry,
      .frame_device_pixel_ratio = 1.0f,
      .raster_cached_entries = &raster_cached_entries,
  };
  layer->Preroll(&preroll_context);

  GrSoftwareSurface surface(kSurfaceSize, kSurfaceSize);
  ASSERT_TRUE(surface.IsValid());
  surface.Clear(Color::kTransparent().Value());
  LayerStateStack paint_state_stack;
  paint_state_stack.set_delegate(surface.GetCanvas());
  PaintContext paint_context{
      .state_stack = paint_state_stack,
      .canvas = surface.GetCanvas(),
      .gr_context = nullptr,
      .compositor_state = nullptr,
      .raster_time = raster_time,
      .ui_time = ui_time,
      .drawable_image_registry = drawable_image_registry,
      .raster_cache = nullptr,
      .frame_device_pixel_ratio = 1.0f,
  };
  layer->Paint(paint_context);
  surface.Flush();

  EXPECT_EQ(surface.GetPixelColor(15, 20), Color::kRed().Value());
  EXPECT_EQ(surface.GetPixelColor(40, 20), Color::kTransparent().Value());
}

}  // namespace
}  // namespace testing
}  // namespace clay
