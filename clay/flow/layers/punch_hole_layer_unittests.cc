// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "base/include/fml/macros.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/punch_hole_layer.h"
#include "clay/flow/testing/diff_context_test.h"
#include "clay/flow/testing/layer_test.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/mock_canvas.h"

namespace clay {
namespace testing {

using PunchHoleLayerTest = LayerTest;

#ifndef NDEBUG
TEST_F(PunchHoleLayerTest, PaintingEmptyLayerDies) {
  skity::Rect empty_rect{};
  auto layer = std::make_shared<PunchHoleLayer>(empty_rect);
  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_TRUE(layer->needs_painting(paint_context()));
  EXPECT_TRUE(preroll_context()->has_punch_hole_layer);
}
#endif

TEST_F(PunchHoleLayerTest, OpacityInheritance) {
  const skity::Rect rect = skity::Rect::MakeXYWH(0, 0, 50, 50);
  auto layer = std::make_shared<PunchHoleLayer>(rect);

  // The punch hole layer always reports opacity compatibility.
  PrerollContext* context = preroll_context();
  layer->Preroll(context);
  EXPECT_EQ(context->renderable_state_flags,
            LayerStateStack::kCallerCanApplyOpacity);
}

TEST_F(PunchHoleLayerTest, PunchHoleRectSize) {
  const skity::Rect rect = skity::Rect::MakeXYWH(0, 0, 50, 50);
  auto layer = std::make_shared<PunchHoleLayer>(rect);
  EXPECT_EQ(layer->PunchHoleRect(), skity::Rect::MakeXYWH(0, 0, 50, 50));
}

TEST_F(PunchHoleLayerTest, AsPunchHoleLayer) {
  const skity::Rect rect = skity::Rect::MakeXYWH(0, 0, 50, 50);
  auto layer1 = std::make_shared<PunchHoleLayer>(rect);
  auto layer2 = std::make_shared<ContainerLayer>();

  EXPECT_TRUE(layer1->as_punch_hole_layer() != nullptr);
  EXPECT_TRUE(layer2->as_punch_hole_layer() == nullptr);
}

TEST_F(PunchHoleLayerTest, PaintNormalRect) {
  const skity::Rect rect = skity::Rect::MakeXYWH(0, 0, 50, 50);
  auto layer = std::make_shared<PunchHoleLayer>(rect);

  layer->Preroll(preroll_context());
  EXPECT_EQ(layer->paint_bounds(), rect);
  EXPECT_TRUE(layer->needs_painting(paint_context()));

  layer->Paint(paint_context());
  SkPaint paint;
  paint.setBlendMode(SkBlendMode::kClear);
  paint.setStyle(SkPaint::kFill_Style);
  const auto expected_draw_calls = std::vector{MockCanvas::DrawCall{
      0,
      MockCanvas::DrawRectData{clay::ConvertSkityRectToSkRect(rect), paint}}};
  EXPECT_EQ(mock_canvas().draw_calls(), expected_draw_calls);
}

using PunchHoleLayerDiffTest = DiffContextTest;

TEST_F(PunchHoleLayerDiffTest, PunchHoleInRetainedLayer) {
  MockLayerTree tree1;
  auto container = std::make_shared<ContainerLayer>();
  tree1.root()->Add(container);
  auto layer = std::make_shared<PunchHoleLayer>(skity::Rect::MakeWH(100, 100));
  container->Add(layer);

  MockLayerTree tree2;
  tree2.root()->Add(container);  // retained layer

  auto damage = DiffLayerTree(tree1, MockLayerTree());
  EXPECT_EQ(damage.frame_damage, skity::Rect::MakeLTRB(0, 0, 100, 100));

  damage = DiffLayerTree(tree2, tree1);
  EXPECT_TRUE(damage.frame_damage.IsEmpty());
}

}  // namespace testing
}  // namespace clay
