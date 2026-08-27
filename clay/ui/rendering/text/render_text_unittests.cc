// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "base/include/fml/thread.h"
#include "clay/fml/icu_util.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/internal_text_view.h"
#include "clay/ui/compositing/frame_builder.h"
#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/compositing/pending_offset_layer.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/renderer.h"
#include "clay/ui/resource/font_collection.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

class CountingRendererClient : public RendererClient {
 public:
  void RequestNewFrame() override {}

  RenderPhase GetRenderPhase() const override { return RenderPhase::kIdle; }

  fml::RefPtr<PaintImage> MakeRasterSnapshot(GrPicturePtr,
                                             skity::Vec2) override {
    ++raster_snapshot_count_;
    return nullptr;
  }

  void RegisterUploadTask(OneShotCallback<>&&, int) override {}

  int raster_snapshot_count() const { return raster_snapshot_count_; }

 private:
  int raster_snapshot_count_ = 0;
};

class RenderTextTest : public ::testing::Test {
 protected:
  RenderTextTest() : thread_("ui") {
    fml::icu::InitializeICU("icudtl.dat");
    FontCollection::Instance()->SetupDefaultFontManager(0);
  }

  fml::Thread thread_;
};

int CountNonTransparentPixels(GrSoftwareSurface& surface, int width,
                              int height) {
  int non_transparent_pixels = 0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (surface.GetPixelAlpha(x, y) != 0) {
        ++non_transparent_pixels;
      }
    }
  }
  return non_transparent_pixels;
}

int CountNonTransparentPixels(const Picture& picture, int width, int height) {
  GrSoftwareSurface surface(width, height);
  EXPECT_TRUE(surface.IsValid());
  if (!surface.IsValid()) {
    return 0;
  }
  surface.Clear(0);
  surface.DrawPicture(picture.picture()->raw());
  surface.Flush();
  return CountNonTransparentPixels(surface, width, height);
}

TEST_F(RenderTextTest, BackgroundClipTextDoesNotRasterizeMaskOnUIThread) {
  auto page_view =
      std::make_unique<PageView>(-1, nullptr, thread_.GetTaskRunner());
  auto text_view = std::make_unique<InternalTextView>(-1, page_view.get());
  text_view->SetText("hello world");
  text_view->SetFontSize(50);
  text_view->SetFontFamily("Roboto");
  text_view->SetPaddings(0.f, 0.f, 0.f, 0.f);

  MeasureResult result;
  text_view->Measure(
      {1000.f, MeasureMode::kAtMost, 1000.f, MeasureMode::kAtMost}, result);

  RenderText* render_text = text_view->GetRenderText();
  render_text->SetWidth(result.width);
  render_text->SetHeight(result.height);
  auto gradient = Gradient::Create("linear-gradient(90deg, red, green)");
  ASSERT_TRUE(gradient.has_value());
  render_text->ResizeBackground(1);
  render_text->SetBackgroundImage(0, *gradient);
  render_text->SetBackgroundClip({ClayBackgroundClipType::kText});

  Renderer* original_renderer = render_text->GetRenderer();
  CountingRendererClient client;
  Renderer renderer(&client, nullptr);
  renderer.SetRoot(render_text);

  PendingContainerLayer root_layer;
  PaintingContext context(&root_layer, render_text, nullptr);
  render_text->Paint(context, FloatPoint(20.f, 30.f));
  context.StopRecordingIfNeeded();

  EXPECT_EQ(client.raster_snapshot_count(), 0);
  render_text->SetRenderer(original_renderer);
}

TEST_F(RenderTextTest, BackgroundClipMaskIgnoresTransparentTextColor) {
  auto page_view =
      std::make_unique<PageView>(-1, nullptr, thread_.GetTaskRunner());
  auto text_view = std::make_unique<InternalTextView>(-1, page_view.get());
  text_view->SetText("hello world");
  text_view->SetFontSize(50);
  text_view->SetFontFamily("Roboto");
  // TTText reserves zero as an undefined color, so use a non-zero RGB value
  // with zero alpha to exercise transparent text on both backends.
  text_view->SetTextColor(Color(0x00FFFFFF));
  text_view->SetPaddings(0.f, 0.f, 0.f, 0.f);

  MeasureResult result;
  text_view->Measure(
      {1000.f, MeasureMode::kAtMost, 1000.f, MeasureMode::kAtMost}, result);

  RenderText* render_text = text_view->GetRenderText();
  auto record_text = [&](bool as_mask) {
    GraphicsContext context(nullptr);
    context.BeginRecording(skity::Rect::MakeWH(result.width, result.height));
    if (as_mask) {
      render_text->GetPainter()->PaintMask(&context);
    } else {
      render_text->GetPainter()->Paint(&context);
    }
    return context.FinishRecording();
  };

  auto normal_picture = record_text(false);
  ASSERT_TRUE(normal_picture);
  EXPECT_EQ(
      CountNonTransparentPixels(*normal_picture, result.width, result.height),
      0);

  auto mask_picture = record_text(true);
  ASSERT_TRUE(mask_picture);
  EXPECT_GT(
      CountNonTransparentPixels(*mask_picture, result.width, result.height), 0);

  // Painting the mask must not mutate the paragraph's normal text style.
  normal_picture = record_text(false);
  EXPECT_EQ(
      CountNonTransparentPixels(*normal_picture, result.width, result.height),
      0);
}

TEST_F(RenderTextTest, BackgroundClipTextRendersThroughPictureMask) {
  auto page_view =
      std::make_unique<PageView>(-1, nullptr, thread_.GetTaskRunner());
  auto text_view = std::make_unique<InternalTextView>(-1, page_view.get());
  text_view->SetText("Hi");
  text_view->SetFontSize(30);
  text_view->SetFontFamily("Roboto");
  text_view->SetTextColor(Color::kTransparent());
  text_view->SetPaddings(0.f, 0.f, 0.f, 0.f);

  MeasureResult result;
  text_view->Measure({50.f, MeasureMode::kAtMost, 50.f, MeasureMode::kAtMost},
                     result);

  RenderText* render_text = text_view->GetRenderText();
  render_text->SetWidth(result.width);
  render_text->SetHeight(result.height);
  auto gradient = Gradient::Create("linear-gradient(90deg, red, green)");
  ASSERT_TRUE(gradient.has_value());
  render_text->ResizeBackground(1);
  render_text->SetBackgroundImage(0, *gradient);
  render_text->SetBackgroundClip({ClayBackgroundClipType::kText});

  Renderer* original_renderer = render_text->GetRenderer();
  CountingRendererClient client;
  Renderer renderer(&client, nullptr);
  renderer.SetRoot(render_text);

  PendingContainerLayer pending_root;
  auto* offset_layer = new PendingOffsetLayer(FloatPoint(10.f, 10.f));
  pending_root.AppendChild(offset_layer);
  PaintingContext painting_context(offset_layer, render_text, nullptr);
  render_text->Paint(painting_context, FloatPoint(5.f, 5.f));
  painting_context.StopRecordingIfNeeded();

  FrameBuilder frame_builder({64.f, 64.f}, 1.f, nullptr);
  frame_builder.BuildFrame(&pending_root);
  auto layer_tree = frame_builder.TakeLayerTree();
  ASSERT_NE(layer_tree, nullptr);
  auto root_layer = layer_tree->root_layer();
  ASSERT_NE(root_layer, nullptr);
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
  root_layer->Preroll(&preroll_context);

  GrSoftwareSurface surface(64, 64);
  ASSERT_TRUE(surface.IsValid());
  auto* canvas = surface.GetCanvas();
  surface.Clear(0);
  LayerStateStack paint_state_stack;
  paint_state_stack.set_delegate(canvas);
  PaintContext paint_context{
      .state_stack = paint_state_stack,
      .canvas = canvas,
      .gr_context = nullptr,
      .compositor_state = nullptr,
      .raster_time = raster_time,
      .ui_time = ui_time,
      .drawable_image_registry = drawable_image_registry,
      .raster_cache = nullptr,
      .frame_device_pixel_ratio = 1.0f,
  };
  root_layer->Paint(paint_context);
  surface.Flush();

  int painted_pixels = CountNonTransparentPixels(surface, 64, 64);
  EXPECT_GT(painted_pixels, 0);
  EXPECT_LT(painted_pixels, static_cast<int>(result.width * result.height));
  render_text->SetRenderer(original_renderer);
}

}  // namespace
}  // namespace clay
