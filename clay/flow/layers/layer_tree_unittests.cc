// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/include/fml/macros.h"
#include "clay/flow/animation/animation_host.h"
#include "clay/flow/animation/animation_mutator.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/layers/clip_rect_layer.h"
#include "clay/flow/layers/color_filter_layer.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/testing/mock_layer.h"
#include "clay/gfx/animation/transition_manager.h"
#include "clay/gfx/paint.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/testing/canvas_test.h"
#include "clay/testing/mock_canvas.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifndef ENABLE_SKITY
#include "third_party/skia/include/core/SkPixmap.h"
#include "third_party/skia/include/core/SkSurface.h"
#endif

namespace clay {
namespace testing {
class LayerTreeTest : public CanvasTest {
 public:
  LayerTreeTest()
      : layer_tree_({64, 64}, 1.0f),
        root_transform_(skity::Matrix::Translate(1.0f, 1.0f)),
        scoped_frame_(compositor_context_.AcquireFrame(
            nullptr, &mock_canvas(), nullptr, root_transform_, false, true)) {}

  LayerTree& layer_tree() { return layer_tree_; }
  CompositorContext::ScopedFrame& frame() { return *scoped_frame_.get(); }
  const skity::Matrix& root_transform() { return root_transform_; }

 private:
  LayerTree layer_tree_;
  CompositorContext compositor_context_;
  skity::Matrix root_transform_;
  std::unique_ptr<CompositorContext::ScopedFrame> scoped_frame_;
};

TEST_F(LayerTreeTest, PaintingEmptyLayerDies) {
  auto layer = std::make_shared<ContainerLayer>();

  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(layer->paint_bounds(), skity::Rect::MakeEmpty());
  EXPECT_TRUE(layer->is_empty());

  layer_tree().Paint(frame());
}

TEST_F(LayerTreeTest, PaintBeforePrerollDies) {
  const SkRect child_bounds = SkRect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  SkPath child_path;
  child_path.addRect(child_bounds);
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer);

  layer_tree().set_root_layer(layer);
  EXPECT_EQ(mock_layer->paint_bounds(), kEmptyRect);
  EXPECT_EQ(layer->paint_bounds(), kEmptyRect);
  EXPECT_TRUE(mock_layer->is_empty());
  EXPECT_TRUE(layer->is_empty());

  layer_tree().Paint(frame());
  EXPECT_EQ(mock_canvas().draw_calls(), std::vector<MockCanvas::DrawCall>());
}

TEST_F(LayerTreeTest, BeginRetainedPrerollWithoutRootLayerIsNoOp) {
  EXPECT_EQ(layer_tree().retained_preroll_generation(), 0u);

  layer_tree().BeginRetainedPreroll();

  EXPECT_EQ(layer_tree().retained_preroll_generation(), 0u);
}

TEST_F(LayerTreeTest, DoAnimationsUsesProvidedFrameTime) {
  auto layer = std::make_shared<OpacityLayer>(0, skity::Vec2{0.0f, 0.0f});
  auto mutator = AnimationMutator::Create(
      ElementId(1), AnimationMutatorType::kOpacity, layer.get());
  auto transition_manager = std::make_unique<TransitionManager>(mutator.get());
  TransitionData transition;
  transition.duration = 1000;
  transition.property = ClayAnimationPropertyType::kOpacity;
  transition_manager->AppendData(transition);
  ASSERT_TRUE(transition_manager->TransitionTo(
      ClayAnimationPropertyType::kOpacity, 1.0f));
  mutator->AddTransitionManager(std::move(transition_manager));

  auto animation_host = std::make_shared<AnimationHost>();
  animation_host->AddAnimationMutator(layer.get(), mutator);
  layer_tree().SetAnimationHost(std::move(animation_host));

  EXPECT_TRUE(mutator->HasAnimationRunning());
  EXPECT_FALSE(layer_tree().DoAnimations(1000));
  EXPECT_FLOAT_EQ(mutator->asOpacity()->alpha(), 0.0f);

  EXPECT_FALSE(layer_tree().DoAnimations(1500));
  EXPECT_FLOAT_EQ(mutator->asOpacity()->alpha(), 127.5f);

  EXPECT_TRUE(layer_tree().DoAnimations(2000));
  EXPECT_FLOAT_EQ(mutator->asOpacity()->alpha(), 255.0f);
  EXPECT_FALSE(mutator->HasAnimationRunning());
}

TEST_F(LayerTreeTest, Simple) {
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  const SkPaint child_paint = SkPaint(SkColors::kCyan);
  auto mock_layer = std::make_shared<MockLayer>(child_path, child_paint);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer);

  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer->paint_bounds(), child_bounds);
  EXPECT_EQ(layer->paint_bounds(), mock_layer->paint_bounds());
  EXPECT_FALSE(mock_layer->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer->parent_matrix(), root_transform());

  layer_tree().Paint(frame());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({MockCanvas::DrawCall{
                0, MockCanvas::DrawPathData{child_path, child_paint}}}));
}

#ifndef ENABLE_SKITY
class LayerTreeRasterTest : public ::testing::Test {
 protected:
  static constexpr int kFrameSize = 64;

  void SetUp() override {
    surface_ = SkSurface::MakeRasterN32Premul(kFrameSize, kFrameSize);
    ASSERT_TRUE(surface_);
  }

  std::unique_ptr<CompositorContext::ScopedFrame> AcquireFrame(
      const skity::Matrix& root_transform = skity::Matrix()) {
    auto* canvas = surface_->getCanvas();
    canvas->setMatrix(ConvertSkityMatrixToSkM44(root_transform));
    return context_.AcquireFrame(nullptr, canvas, nullptr, root_transform,
                                 false, true);
  }

  void ExpectPixel(int x, int y, SkColor color) {
    SkPixmap pixels;
    ASSERT_TRUE(surface_->peekPixels(&pixels));
    EXPECT_EQ(pixels.getColor(x, y), color);
  }

  static void SetRootWithChild(LayerTree& tree, std::shared_ptr<Layer> child) {
    auto root = std::make_shared<ContainerLayer>();
    root->Add(std::move(child));
    tree.set_root_layer(root);
  }

  static std::shared_ptr<MockLayer> MakeRect(const skity::Rect& rect,
                                             SkColor color) {
    SkPaint paint;
    paint.setColor(color);
    return std::make_shared<MockLayer>(
        SkPath().addRect(ConvertSkityRectToSkRect(rect)), paint);
  }

 private:
  sk_sp<SkSurface> surface_;
  CompositorContext context_;
};

TEST_F(LayerTreeRasterTest, PartialRepaintPreservesRasterCacheContents) {
  const auto target = skity::Rect::MakeLTRB(28, 5, 36, 15);
  auto clip = std::make_shared<ClipRectLayer>(target, Clip::hardEdge);
  clip->Add(MakeRect(target, SK_ColorRED));

  auto effect = std::make_shared<ColorFilterLayer>(
      SrgbToLinearGammaColorFilter::GetInstance());
  effect->Add(MakeRect(skity::Rect::MakeLTRB(5, 5, 15, 15), SK_ColorBLUE));
  effect->Add(clip);
  effect->Add(MakeRect(skity::Rect::MakeLTRB(45, 5, 55, 15), SK_ColorGREEN));

  auto frame = AcquireFrame();
  ASSERT_TRUE(frame);
  auto& cache = frame->context().raster_cache();

  // Populate the framebuffer while leaving the cache cold.
  LayerTree initial_tree({kFrameSize, kFrameSize}, 1.0f);
  SetRootWithChild(initial_tree, effect);
  FrameDamage initial_damage;
  ASSERT_EQ(frame->Raster(initial_tree, true, &initial_damage, SK_ColorWHITE),
            RasterStatus::kSuccess);
  ExpectPixel(32, 10, SK_ColorRED);
  ASSERT_EQ(cache.GetLayerCachedEntriesCount(), 0u);

  // The cache still needs the whole effect subtree when only its left side
  // has framebuffer damage.
  LayerTree cached_tree({kFrameSize, kFrameSize}, 1.0f);
  SetRootWithChild(cached_tree, effect);
  const auto left = skity::Rect::MakeLTRB(0, 0, 20, 20);
  FrameDamage left_damage;
  left_damage.SetPreviousLayerTree(&initial_tree);
  left_damage.AddAdditionalDamage(left);
  cache.BeginFrame();
  ASSERT_EQ(frame->Raster(cached_tree, false, &left_damage, SK_ColorWHITE),
            RasterStatus::kSuccess);
  EXPECT_EQ(left_damage.GetBufferDamage(), left);
  ASSERT_EQ(cache.GetLayerCachedEntriesCount(), 1u);
  cache.EndFrame();
  ExpectPixel(32, 10, SK_ColorRED);

  // Redraw the middle from the cache populated by the previous frame.
  LayerTree repaint_tree({kFrameSize, kFrameSize}, 1.0f);
  SetRootWithChild(repaint_tree, effect);
  FrameDamage target_damage;
  target_damage.SetPreviousLayerTree(&cached_tree);
  target_damage.AddAdditionalDamage(target);
  cache.BeginFrame();
  ASSERT_EQ(frame->Raster(repaint_tree, false, &target_damage, SK_ColorWHITE),
            RasterStatus::kSuccess);
  EXPECT_EQ(target_damage.GetBufferDamage(), target);
  EXPECT_EQ(cache.GetLayerCachedEntriesCount(), 1u);
  cache.EndFrame();
  EXPECT_EQ(clip->paint_bounds(), target);
  ExpectPixel(32, 10, SK_ColorRED);
}

TEST_F(LayerTreeRasterTest, PartialRepaintMapsDamageForFlippedRoot) {
  const auto child_bounds = skity::Rect::MakeLTRB(5, 5, 15, 15);
  const auto clip_bounds = skity::Rect::MakeLTRB(3, 3, 24, 24);
  const auto damage_rect = skity::Rect::MakeLTRB(0, 0, 20, 20);
  const skity::Matrix flip_y(1, 0, 0, 0, -1, kFrameSize, 0, 0, 1);
  auto frame = AcquireFrame(flip_y);
  ASSERT_TRUE(frame);

  LayerTree initial_tree({kFrameSize, kFrameSize}, 1.0f);
  auto initial_clip =
      std::make_shared<ClipRectLayer>(clip_bounds, Clip::hardEdge);
  initial_clip->Add(MakeRect(child_bounds, SK_ColorRED));
  SetRootWithChild(initial_tree, initial_clip);
  FrameDamage initial_damage;
  ASSERT_EQ(frame->Raster(initial_tree, true, &initial_damage, SK_ColorWHITE),
            RasterStatus::kSuccess);
  ExpectPixel(10, kFrameSize - 10, SK_ColorRED);

  LayerTree repaint_tree({kFrameSize, kFrameSize}, 1.0f);
  auto child = MakeRect(child_bounds, SK_ColorRED);
  auto clip = std::make_shared<ClipRectLayer>(clip_bounds, Clip::hardEdge);
  clip->Add(child);
  clip->AssignOldLayer(initial_clip.get());
  SetRootWithChild(repaint_tree, clip);
  FrameDamage damage;
  damage.SetPreviousLayerTree(&initial_tree);
  damage.AddAdditionalDamage(damage_rect);
  ASSERT_EQ(frame->Raster(repaint_tree, true, &damage, SK_ColorBLACK),
            RasterStatus::kSuccess);

  EXPECT_EQ(damage.GetBufferDamage(), damage_rect);
  EXPECT_EQ(child->parent_cull_rect(), skity::Rect::MakeLTRB(3, 3, 20, 20));
  EXPECT_EQ(clip->paint_bounds(), child_bounds);
  ExpectPixel(10, kFrameSize - 10, SK_ColorRED);
  ExpectPixel(2, kFrameSize - 10, SK_ColorBLACK);
  ExpectPixel(25, kFrameSize - 10, SK_ColorWHITE);
}
#endif

TEST_F(LayerTreeTest, RasterPreservesBoundsOutsideFrame) {
  const skity::Rect clip_bounds =
      skity::Rect::MakeXYWH(100.0f, 100.0f, 10.0f, 10.0f);
  const SkPath child_path =
      SkPath().addRect(clip_bounds.Left(), clip_bounds.Top(),
                       clip_bounds.Right(), clip_bounds.Bottom());
  auto mock_layer = std::make_shared<MockLayer>(child_path);
  auto clip_layer =
      std::make_shared<ClipRectLayer>(clip_bounds, Clip::hardEdge);
  clip_layer->Add(mock_layer);

  layer_tree().set_root_layer(clip_layer);
  frame().Raster(layer_tree(), true, nullptr);

  EXPECT_EQ(mock_layer->paint_bounds(), clip_bounds);
  EXPECT_EQ(clip_layer->paint_bounds(), clip_bounds);
  EXPECT_EQ(clip_layer->child_paint_bounds(), clip_bounds);
}

TEST_F(LayerTreeTest, RetainedPrerollPreservesBoundsOutsideDamage) {
  const skity::Rect child_bounds =
      skity::Rect::MakeLTRB(40.0f, 40.0f, 50.0f, 50.0f);
  const SkPath child_path =
      SkPath().addRect(clay::ConvertSkityRectToSkRect(child_bounds));
  auto child = std::make_shared<MockCacheableLayer>(child_path, SkPaint(), 1);
  auto retained_clip = std::make_shared<ClipRectLayer>(
      skity::Rect::MakeLTRB(32.0f, 32.0f, 56.0f, 56.0f), Clip::hardEdge);
  retained_clip->Add(child);

  LayerTree visible_tree({64, 64}, 1.0f);
  auto visible_root = std::make_shared<ContainerLayer>();
  visible_root->Add(retained_clip);
  visible_tree.set_root_layer(visible_root);

  auto& raster_cache = frame().context().raster_cache();
  const RasterCacheKeyID child_cache_key(child->unique_id(),
                                         RasterCacheKeyType::kLayer);

  raster_cache.BeginFrame();
  FrameDamage initial_damage;
  EXPECT_EQ(frame().Raster(visible_tree, false, &initial_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(child->preroll_count(), 1u);
  ASSERT_EQ(retained_clip->paint_bounds(), child_bounds);
  EXPECT_EQ(raster_cache.GetAccessCount(child_cache_key, root_transform()), 1);
  raster_cache.EndFrame();

  LayerTree outside_tree({64, 64}, 1.0f);
  auto outside_root = std::make_shared<ContainerLayer>();
  outside_root->Add(retained_clip);
  outside_tree.set_root_layer(outside_root);

  raster_cache.BeginFrame();
  FrameDamage outside_damage;
  outside_damage.SetPreviousLayerTree(&visible_tree);
  outside_damage.AddAdditionalDamage(
      skity::Rect::MakeLTRB(8.0f, 8.0f, 12.0f, 12.0f));
  EXPECT_EQ(frame().Raster(outside_tree, false, &outside_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(child->preroll_count(), 2u);
  ASSERT_EQ(retained_clip->paint_bounds(), child_bounds);
  EXPECT_EQ(raster_cache.GetAccessCount(child_cache_key, root_transform()), -1);
  raster_cache.EndFrame();

  LayerTree visible_again_tree({64, 64}, 1.0f);
  auto visible_again_root = std::make_shared<ContainerLayer>();
  visible_again_root->Add(retained_clip);
  visible_again_tree.set_root_layer(visible_again_root);

  raster_cache.BeginFrame();
  FrameDamage visible_damage;
  visible_damage.SetPreviousLayerTree(&outside_tree);
  visible_damage.AddAdditionalDamage(child_bounds);
  EXPECT_EQ(frame().Raster(visible_again_tree, false, &visible_damage),
            RasterStatus::kSuccess);
  EXPECT_EQ(child->preroll_count(), 3u);
  EXPECT_EQ(child->paint_bounds(), child_bounds);
  EXPECT_EQ(retained_clip->paint_bounds(), child_bounds);
  EXPECT_EQ(raster_cache.GetAccessCount(child_cache_key, root_transform()), 1);
  raster_cache.EndFrame();
}

TEST_F(LayerTreeTest, RetainedPrerollSkipsChildrenOutsideDamage) {
  const SkPath damaged_path = SkPath().addRect(5.0f, 5.0f, 20.0f, 20.0f);
  const SkPath retained_path = SkPath().addRect(40.0f, 40.0f, 50.0f, 50.0f);
  auto damaged_child = std::make_shared<MockLayer>(damaged_path);
  auto retained_child = std::make_shared<MockLayer>(retained_path);
  auto retained_container = std::make_shared<ContainerLayer>();
  retained_container->Add(damaged_child);
  retained_container->Add(retained_child);

  LayerTree previous_tree({64, 64}, 1.0f);
  auto previous_root = std::make_shared<ContainerLayer>();
  previous_root->Add(retained_container);
  previous_tree.set_root_layer(previous_root);

  FrameDamage initial_damage;
  EXPECT_EQ(frame().Raster(previous_tree, false, &initial_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(damaged_child->preroll_count(), 1u);
  ASSERT_EQ(retained_child->preroll_count(), 1u);

  LayerTree current_tree({64, 64}, 1.0f);
  auto current_root = std::make_shared<ContainerLayer>();
  current_root->Add(retained_container);
  current_tree.set_root_layer(current_root);

  FrameDamage partial_damage;
  partial_damage.SetPreviousLayerTree(&previous_tree);
  partial_damage.AddAdditionalDamage(
      skity::Rect::MakeLTRB(8.0f, 8.0f, 12.0f, 12.0f));
  EXPECT_EQ(frame().Raster(current_tree, false, &partial_damage),
            RasterStatus::kSuccess);

  EXPECT_EQ(damaged_child->preroll_count(), 2u);
  EXPECT_EQ(retained_child->preroll_count(), 1u);
  EXPECT_EQ(retained_container->paint_bounds(),
            skity::Rect::MakeLTRB(5.0f, 5.0f, 50.0f, 50.0f));
}

TEST_F(LayerTreeTest, RetainedPrerollDoesNotSkipPlatformViewSubtree) {
  const SkPath child_path = SkPath().addRect(40.0f, 40.0f, 50.0f, 50.0f);
  auto child = std::make_shared<MockLayer>(child_path);
  child->set_fake_has_platform_view(true);

  LayerTree previous_tree({64, 64}, 1.0f);
  auto previous_root = std::make_shared<ContainerLayer>();
  previous_root->Add(child);
  previous_tree.set_root_layer(previous_root);

  FrameDamage initial_damage;
  EXPECT_EQ(frame().Raster(previous_tree, false, &initial_damage),
            RasterStatus::kSuccess);
  ASSERT_EQ(child->preroll_count(), 1u);

  LayerTree current_tree({64, 64}, 1.0f);
  auto current_root = std::make_shared<ContainerLayer>();
  current_root->Add(child);
  current_tree.set_root_layer(current_root);

  FrameDamage partial_damage;
  partial_damage.SetPreviousLayerTree(&previous_tree);
  partial_damage.AddAdditionalDamage(
      skity::Rect::MakeLTRB(8.0f, 8.0f, 12.0f, 12.0f));
  EXPECT_EQ(frame().Raster(current_tree, false, &partial_damage),
            RasterStatus::kSuccess);

  EXPECT_EQ(child->preroll_count(), 2u);
}

TEST_F(LayerTreeTest, Multiple) {
  const SkPath child_path1 = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path2 = SkPath().addRect(8.0f, 2.0f, 16.5f, 14.5f);
  const SkPaint child_paint1(SkColors::kGray);
  const SkPaint child_paint2(SkColors::kGreen);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  mock_layer1->set_fake_has_platform_view(true);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  skity::Rect expected_total_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  expected_total_bounds.Join(
      clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_total_bounds);
  EXPECT_FALSE(mock_layer1->is_empty());
  EXPECT_FALSE(mock_layer2->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer1->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer2->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(),
            kGiantRect);  // Siblings are independent

  layer_tree().Paint(frame());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0, MockCanvas::DrawPathData{child_path1, child_paint1}},
                   MockCanvas::DrawCall{0, MockCanvas::DrawPathData{
                                               child_path2, child_paint2}}}));
}

TEST_F(LayerTreeTest, MultipleWithEmpty) {
  const SkPath child_path1 = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPaint child_paint1(SkColors::kGray);
  const SkPaint child_paint2(SkColors::kGreen);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(SkPath(), child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(SkPath().getBounds()));
  EXPECT_EQ(layer->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_FALSE(mock_layer1->is_empty());
  EXPECT_TRUE(mock_layer2->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer1->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer2->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(), kGiantRect);

  layer_tree().Paint(frame());
  EXPECT_EQ(mock_canvas().draw_calls(),
            std::vector({MockCanvas::DrawCall{
                0, MockCanvas::DrawPathData{child_path1, child_paint1}}}));
}

TEST_F(LayerTreeTest, NeedsSystemComposite) {
  const SkPath child_path1 = SkPath().addRect(5.0f, 6.0f, 20.5f, 21.5f);
  const SkPath child_path2 = SkPath().addRect(8.0f, 2.0f, 16.5f, 14.5f);
  const SkPaint child_paint1(SkColors::kGray);
  const SkPaint child_paint2(SkColors::kGreen);
  auto mock_layer1 = std::make_shared<MockLayer>(child_path1, child_paint1);
  auto mock_layer2 = std::make_shared<MockLayer>(child_path2, child_paint2);
  auto layer = std::make_shared<ContainerLayer>();
  layer->Add(mock_layer1);
  layer->Add(mock_layer2);

  skity::Rect expected_total_bounds =
      clay::ConvertSkRectToSkityRect(child_path1.getBounds());
  expected_total_bounds.Join(
      clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  layer_tree().set_root_layer(layer);
  layer_tree().Preroll(frame());
  EXPECT_EQ(mock_layer1->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path1.getBounds()));
  EXPECT_EQ(mock_layer2->paint_bounds(),
            clay::ConvertSkRectToSkityRect(child_path2.getBounds()));
  EXPECT_EQ(layer->paint_bounds(), expected_total_bounds);
  EXPECT_FALSE(mock_layer1->is_empty());
  EXPECT_FALSE(mock_layer2->is_empty());
  EXPECT_FALSE(layer->is_empty());
  EXPECT_EQ(mock_layer1->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer2->parent_matrix(), root_transform());
  EXPECT_EQ(mock_layer1->parent_cull_rect(), kGiantRect);
  EXPECT_EQ(mock_layer2->parent_cull_rect(), kGiantRect);

  layer_tree().Paint(frame());
  EXPECT_EQ(
      mock_canvas().draw_calls(),
      std::vector({MockCanvas::DrawCall{
                       0, MockCanvas::DrawPathData{child_path1, child_paint1}},
                   MockCanvas::DrawCall{0, MockCanvas::DrawPathData{
                                               child_path2, child_paint2}}}));
}

TEST_F(LayerTreeTest, PrerollContextInitialization) {
  LayerStateStack state_stack;
  state_stack.set_preroll_delegate(kGiantRect, skity::Matrix());
  FixedRefreshRateStopwatch mock_raster_time;
  FixedRefreshRateStopwatch mock_ui_time;
  std::shared_ptr<DrawableImageRegistry> mock_registry;

  auto expect_defaults = [&state_stack, &mock_raster_time, &mock_ui_time,
                          &mock_registry](const PrerollContext& context) {
    EXPECT_EQ(context.raster_cache, nullptr);
    EXPECT_EQ(context.gr_context, nullptr);
    EXPECT_EQ(context.compositor_state, nullptr);
    EXPECT_EQ(&context.state_stack, &state_stack);
    EXPECT_EQ(context.dst_color_space, nullptr);
    EXPECT_EQ(context.state_stack.device_cull_rect(), kGiantRect);
    EXPECT_EQ(context.state_stack.transform_4x4(), skity::Matrix());
    EXPECT_EQ(context.surface_needs_readback, false);

    EXPECT_EQ(&context.raster_time, &mock_raster_time);
    EXPECT_EQ(&context.ui_time, &mock_ui_time);
    EXPECT_EQ(context.drawable_image_registry.get(), mock_registry.get());
    EXPECT_EQ(context.frame_device_pixel_ratio, 1.0f);

    EXPECT_EQ(context.has_platform_view, false);
    EXPECT_EQ(context.has_drawable_image_layer, false);
    EXPECT_EQ(context.has_punch_hole_layer, false);

    EXPECT_EQ(context.renderable_state_flags, 0);
    EXPECT_EQ(context.raster_cached_entries, nullptr);
  };

  // These 4 initializers are required because they are handled by reference
  PrerollContext context{
      .state_stack = state_stack,
      .raster_time = mock_raster_time,
      .ui_time = mock_ui_time,
      .drawable_image_registry = mock_registry,
  };
  expect_defaults(context);
}

TEST_F(LayerTreeTest, PaintContextInitialization) {
  LayerStateStack state_stack;
  FixedRefreshRateStopwatch mock_raster_time;
  FixedRefreshRateStopwatch mock_ui_time;
  std::shared_ptr<DrawableImageRegistry> mock_registry;

  auto expect_defaults = [&state_stack, &mock_raster_time, &mock_ui_time,
                          &mock_registry](const PaintContext& context) {
    EXPECT_EQ(&context.state_stack, &state_stack);
    EXPECT_EQ(context.canvas, nullptr);
    EXPECT_EQ(context.gr_context, nullptr);
    EXPECT_EQ(context.compositor_state, nullptr);
    EXPECT_EQ(&context.raster_time, &mock_raster_time);
    EXPECT_EQ(&context.ui_time, &mock_ui_time);
    EXPECT_EQ(context.drawable_image_registry.get(), mock_registry.get());
    EXPECT_EQ(context.raster_cache, nullptr);
    EXPECT_EQ(context.state_stack.checkerboard_func(), nullptr);
    EXPECT_EQ(context.frame_device_pixel_ratio, 1.0f);
  };

  // These 4 initializers are required because they are handled by reference
  PaintContext context{
      .state_stack = state_stack,
      .raster_time = mock_raster_time,
      .ui_time = mock_ui_time,
      .drawable_image_registry = mock_registry,
  };
  expect_defaults(context);
}

}  // namespace testing
}  // namespace clay
