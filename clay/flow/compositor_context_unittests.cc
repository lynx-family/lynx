// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/flow/compositor_context.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/layers/layer_tree.h"
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

}  // namespace
}  // namespace testing
}  // namespace clay
