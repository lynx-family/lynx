// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/flow/layers/platform_view_layer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {
namespace {

std::unique_ptr<LayerTree> MakeEmptyLayerTree(const skity::Vec2& frame_size) {
  auto tree = std::make_unique<LayerTree>(frame_size, 1.0f);
  tree->set_root_layer(std::make_shared<ContainerLayer>());
  return tree;
}

TEST(FrameDamageTest, WithoutPreviousLayerTreeRepaintsFullFrame) {
  auto layer_tree = MakeEmptyLayerTree({100, 80});
  FrameDamage damage;

  EXPECT_EQ(damage.ComputeClipRect(*layer_tree, true),
            skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(damage.GetFrameDamage(), skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(damage.GetBufferDamage(), skity::Rect::MakeWH(100, 80));
}

TEST(FrameDamageTest, PreviousLayerTreeUsesAdditionalDamageAndAlignment) {
  auto previous_layer_tree = MakeEmptyLayerTree({100, 80});
  FrameDamage initial_damage;
  initial_damage.ComputeClipRect(*previous_layer_tree, true);

  auto layer_tree = MakeEmptyLayerTree({100, 80});
  FrameDamage damage;
  damage.SetPreviousLayerTree(previous_layer_tree.get());
  damage.AddAdditionalDamage(skity::Rect::MakeLTRB(10, 10, 20, 20));
  damage.SetClipAlignment(8, 8);

  EXPECT_EQ(damage.ComputeClipRect(*layer_tree, true),
            skity::Rect::MakeLTRB(8, 8, 24, 24));
  EXPECT_EQ(damage.GetFrameDamage(), skity::Rect::MakeEmpty());
  EXPECT_EQ(damage.GetBufferDamage(), skity::Rect::MakeLTRB(8, 8, 24, 24));
}

TEST(FrameDamageTest, CurrentPlatformViewForcesFullFrame) {
  auto previous_layer_tree = MakeEmptyLayerTree({100, 80});
  FrameDamage initial_damage;
  initial_damage.ComputeClipRect(*previous_layer_tree, true);

  auto layer_tree = MakeEmptyLayerTree({100, 80});
  layer_tree->SetHasPlatformViewLayer(true);
  FrameDamage damage;
  damage.SetPreviousLayerTree(previous_layer_tree.get());
  damage.AddAdditionalDamage(skity::Rect::MakeLTRB(10, 10, 20, 20));

  EXPECT_EQ(damage.ComputeClipRect(*layer_tree, true),
            skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(damage.GetFrameDamage(), skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(damage.GetBufferDamage(), skity::Rect::MakeWH(100, 80));
}

TEST(FrameDamageTest, PreviousPlatformViewForcesFullFrame) {
  auto previous_layer_tree = MakeEmptyLayerTree({100, 80});
  previous_layer_tree->SetHasPlatformViewLayer(true);
  FrameDamage initial_damage;
  initial_damage.ComputeClipRect(*previous_layer_tree, true);

  auto layer_tree = MakeEmptyLayerTree({100, 80});
  FrameDamage damage;
  damage.SetPreviousLayerTree(previous_layer_tree.get());
  damage.AddAdditionalDamage(skity::Rect::MakeLTRB(10, 10, 20, 20));

  EXPECT_EQ(damage.ComputeClipRect(*layer_tree, true),
            skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(damage.GetFrameDamage(), skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(damage.GetBufferDamage(), skity::Rect::MakeWH(100, 80));
}

TEST(FrameDamageTest, PrerollPreservesPlatformViewPresenceForRemoval) {
  auto with_platform_view = MakeEmptyLayerTree({100, 80});
  auto platform_view = std::make_shared<PlatformViewLayer>(
      skity::Vec2{48, 48}, skity::Vec2{40, 20}, 1);
  with_platform_view->set_root_layer(platform_view);
  // A retained subtree can be submitted before its first raster preroll has
  // populated the UI-side platform-view summary.
  EXPECT_FALSE(with_platform_view->HasPlatformViewLayer());
  CompositorContext context;
  CompositorState state({100, 80});
  auto frame = context.AcquireFrame(nullptr, nullptr, &state, skity::Matrix(),
                                    false, true);
  with_platform_view->Preroll(*frame, true, skity::Rect::MakeWH(100, 80));
  ASSERT_FALSE(state.GetCompositionOrder().empty());
  EXPECT_TRUE(with_platform_view->HasPlatformViewLayer());

  auto without_platform_view = MakeEmptyLayerTree({100, 80});
  FrameDamage removal_damage;
  removal_damage.SetPreviousLayerTree(with_platform_view.get());
  EXPECT_EQ(removal_damage.ComputeClipRect(*without_platform_view, true),
            skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(removal_damage.GetFrameDamage(), skity::Rect::MakeWH(100, 80));
}

TEST(FrameDamageTest,
     PlatformViewRemovalRepaintsOlderBuffersBeforePartialDraw) {
  auto with_platform_view = MakeEmptyLayerTree({100, 80});
  with_platform_view->SetHasPlatformViewLayer(true);
  FrameDamage initial_damage;
  initial_damage.ComputeClipRect(*with_platform_view, true);

  auto without_platform_view = MakeEmptyLayerTree({100, 80});
  FrameDamage removal_damage;
  removal_damage.SetPreviousLayerTree(with_platform_view.get());
  EXPECT_EQ(removal_damage.ComputeClipRect(*without_platform_view, true),
            skity::Rect::MakeWH(100, 80));

  // A different back buffer may still contain the removed native view even
  // though both trees now lack platform views. Its accumulated damage must
  // clear those pixels without reporting a new full-frame content change.
  auto next_layer_tree = MakeEmptyLayerTree({100, 80});
  FrameDamage older_buffer_damage;
  older_buffer_damage.SetPreviousLayerTree(without_platform_view.get());
  older_buffer_damage.AddAdditionalDamage(*removal_damage.GetFrameDamage());
  EXPECT_EQ(older_buffer_damage.ComputeClipRect(*next_layer_tree, true),
            skity::Rect::MakeWH(100, 80));
  EXPECT_EQ(older_buffer_damage.GetFrameDamage(), skity::Rect::MakeEmpty());

  // Once the buffer is current, a small change can use partial repaint again.
  auto final_layer_tree = MakeEmptyLayerTree({100, 80});
  FrameDamage partial_damage;
  partial_damage.SetPreviousLayerTree(next_layer_tree.get());
  partial_damage.AddAdditionalDamage(skity::Rect::MakeLTRB(10, 10, 20, 20));
  EXPECT_EQ(partial_damage.ComputeClipRect(*final_layer_tree, true),
            skity::Rect::MakeLTRB(10, 10, 20, 20));
}

}  // namespace
}  // namespace testing
}  // namespace clay
