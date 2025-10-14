// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/macros.h"
#include "clay/flow/layers/drawable_image_layer.h"
#include "clay/flow/testing/diff_context_test.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_drawable_image.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/testing/mock_canvas.h"

namespace clay {
namespace testing {

using DrawableImageLayerTest = LayerTest;

TEST_F(DrawableImageLayerTest, InvalidDrawableImage) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  auto layer = std::make_shared<DrawableImageLayer>(
      layer_offset, layer_size, 0, false, DlImageSampling::kNearestNeighbor);

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(),
            (skity::Rect::MakeSize({layer_size.x, layer_size.y})
                 .MakeOffset(layer_offset.x, layer_offset.y)));
  EXPECT_TRUE(layer->needs_painting(paint_context()));

  layer->Paint(paint_context());
  EXPECT_EQ(mock_canvas().draw_calls(), std::vector<MockCanvas::DrawCall>());
}

#ifndef NDEBUG
TEST_F(DrawableImageLayerTest, PaintingEmptyLayerDies) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(0.0f, 0.0f);
  auto mock_image = std::make_shared<MockDrawableImage>();
  const int64_t image_id = mock_image->Id();
  auto layer = std::make_shared<DrawableImageLayer>(
      layer_offset, layer_size, image_id, false,
      DlImageSampling::kNearestNeighbor);

  // Ensure the image is located by the Layer.
  preroll_context()->drawable_image_registry->RegisterDrawableImage(mock_image);

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_FALSE(layer->needs_painting(paint_context()));

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}

TEST_F(DrawableImageLayerTest, PaintBeforePrerollDies) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  auto mock_image = std::make_shared<MockDrawableImage>();
  const int64_t image_id = mock_image->Id();
  auto layer = std::make_shared<DrawableImageLayer>(
      layer_offset, layer_size, image_id, false, DlImageSampling::kLinear);

  // Ensure the image is located by the Layer.
  preroll_context()->drawable_image_registry->RegisterDrawableImage(mock_image);

  EXPECT_DEATH_IF_SUPPORTED(layer->Paint(paint_context()),
                            "needs_painting\\(context\\)");
}
#endif

TEST_F(DrawableImageLayerTest, PaintingWithLinearSampling) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  auto mock_image = std::make_shared<MockDrawableImage>();
  const int64_t image_id = mock_image->Id();
  auto layer = std::make_shared<DrawableImageLayer>(
      layer_offset, layer_size, image_id, false, DlImageSampling::kLinear);

  // Ensure the image is located by the Layer.
  preroll_context()->drawable_image_registry->RegisterDrawableImage(mock_image);

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(),
            (skity::Rect::MakeSize({layer_size.x, layer_size.y})
                 .MakeOffset(layer_offset.x, layer_offset.y)));
  EXPECT_TRUE(layer->needs_painting(paint_context()));

  layer->Paint(paint_context());
  EXPECT_EQ(mock_image->paint_calls(),
            std::vector({MockDrawableImage::PaintCall{
                mock_canvas(), layer->paint_bounds(), false, nullptr,
                SkSamplingOptions(SkFilterMode::kLinear), nullptr,
                DrawableImage::FitMode::kScaleToFill}}));
  EXPECT_EQ(mock_canvas().draw_calls(), std::vector<MockCanvas::DrawCall>());
}

using DrawableImageLayerDiffTest = DiffContextTest;

TEST_F(DrawableImageLayerDiffTest, DrawableImageInRetainedLayer) {
  MockLayerTree tree1;
  auto container = std::make_shared<ContainerLayer>();
  tree1.root()->Add(container);
  auto layer = std::make_shared<DrawableImageLayer>(
      skity::Vec2(0, 0), skity::Vec2(100, 100), 0, false,
      DlImageSampling::kLinear);
  container->Add(layer);

  MockLayerTree tree2;
  tree2.root()->Add(container);  // retained layer

  auto damage = DiffLayerTree(tree1, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 100, 100));

  damage = DiffLayerTree(tree2, tree1);
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 100, 100));
}

TEST_F(DrawableImageLayerTest, OpacityInheritance) {
  const skity::Vec2 layer_offset = skity::Vec2(0.0f, 0.0f);
  const skity::Vec2 layer_size = skity::Vec2(8.0f, 8.0f);
  auto mock_image = std::make_shared<MockDrawableImage>();
  const int64_t image_id = mock_image->Id();
  auto layer = std::make_shared<DrawableImageLayer>(
      layer_offset, layer_size, image_id, false, DlImageSampling::kLinear);

  // Ensure the image is located by the Layer.
  preroll_context()->drawable_image_registry->RegisterDrawableImage(mock_image);

  // The drawable image layer always reports opacity compatibility.
  PrerollContext* context = preroll_context();
  context->drawable_image_registry->RegisterDrawableImage(mock_image);
  layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);

  // MockDrawableImage has no actual textur to render into the
  // PaintContext canvas so we have no way to verify its
  // rendering.
}

}  // namespace testing
}  // namespace clay
