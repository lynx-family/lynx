// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/path_cache.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {

TEST(FragmentPathCacheTest, ReusesGeometryAcrossPlacementAndInvalidatesShape) {
  FragmentPathCache cache;
  RoundedRectangle box;
  box.SetWidth(100);
  box.SetHeight(60);
  box.SetRadiusXTopLeft(10);
  box.SetRadiusYTopLeft(15);
  auto original = cache.Get(FragmentPathCache::Slot::kPadding, box);
  ASSERT_TRUE(original);
  box.SetX(200);
  box.SetY(300);
  EXPECT_EQ(cache.Get(FragmentPathCache::Slot::kPadding, box), original);
  box.SetRadiusYTopLeft(20);
  auto changed = cache.Get(FragmentPathCache::Slot::kPadding, box);
  EXPECT_NE(changed, original);
  EXPECT_FLOAT_EQ(original->GetHeader().radii[1], 15);
  box.SetWidth(0);
  auto empty = cache.Get(FragmentPathCache::Slot::kPadding, box);
  ASSERT_TRUE(empty);
  EXPECT_TRUE(empty->IsEmpty());
  box.SetWidth(100);
  EXPECT_NE(cache.Get(FragmentPathCache::Slot::kPadding, box), empty);
}

TEST(FragmentPathCacheTest, UpdatingOneBoxPreservesOtherSlots) {
  FragmentPathCache cache;
  RoundedRectangle box;
  box.SetWidth(10);
  box.SetHeight(20);
  auto border = cache.Get(FragmentPathCache::Slot::kBorder, box);
  cache.Get(FragmentPathCache::Slot::kOverflow, box);
  box.SetHeight(30);
  cache.Get(FragmentPathCache::Slot::kOverflow, box);
  box.SetHeight(20);
  EXPECT_EQ(cache.Get(FragmentPathCache::Slot::kBorder, box), border);
}

}  // namespace lynx::tasm
