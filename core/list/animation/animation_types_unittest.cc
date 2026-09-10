// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/animation_types.h"

#include <cstdint>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {
namespace {

// Verify that removal runs first, followed by concurrent move and change,
// then addition.
TEST(UpdateAnimationConfigTest, BuildsDefaultStagesWithIndependentDurations) {
  const std::vector<AnimationStageEntries> expected{
      {{ItemAnimationType::kDisappearance, 120}},
      {{ItemAnimationType::kPersistence, 250},
       {ItemAnimationType::kChange, 250}},
      {{ItemAnimationType::kAppearance, 120}},
  };
  EXPECT_EQ(MakeDefaultAnimationStages(), expected);

  // Custom durations map to types regardless of entry order in the default
  // stages.
  const std::vector<AnimationStageEntries> custom{
      {{ItemAnimationType::kDisappearance, 11}},
      {{ItemAnimationType::kPersistence, 22}, {ItemAnimationType::kChange, 44}},
      {{ItemAnimationType::kAppearance, 33}},
  };
  EXPECT_EQ(MakeDefaultAnimationStages(11, 22, 33, 44), custom);
}

// Verifies that PositionChanged() compares only position and uses the standard
// floating-point tolerance.
TEST(ItemLayoutInfoTest, DetectsPositionChangeWithTolerance) {
  ItemLayoutInfo layout_info;
  layout_info.left_ = 10.f;
  layout_info.top_ = 20.f;
  layout_info.right_ = 40.f;
  layout_info.bottom_ = 60.f;

  // 1. Position differences below EPSILON are not treated as changes.
  ItemLayoutInfo nearly_equal = layout_info;
  nearly_equal.left_ += 0.005f;
  nearly_equal.top_ += 0.005f;
  EXPECT_FALSE(layout_info.PositionChanged(nearly_equal));

  // 2. Horizontal or vertical offsets clearly above the tolerance are detected.
  ItemLayoutInfo horizontal_change = layout_info;
  horizontal_change.left_ += 0.02f;
  EXPECT_TRUE(layout_info.PositionChanged(horizontal_change));

  ItemLayoutInfo vertical_change = layout_info;
  vertical_change.top_ += 0.02f;
  EXPECT_TRUE(layout_info.PositionChanged(vertical_change));
}

// Verifies that BoundsChanged() detects size-related edge changes as well as
// position changes.
TEST(ItemLayoutInfoTest, DetectsBoundsChangeIncludingSize) {
  ItemLayoutInfo layout_info;
  layout_info.left_ = 10.f;
  layout_info.top_ = 20.f;
  layout_info.right_ = 40.f;
  layout_info.bottom_ = 60.f;

  // 1. Changing only right represents a width change, not a position change.
  ItemLayoutInfo width_change = layout_info;
  width_change.right_ += 0.02f;
  EXPECT_FALSE(layout_info.PositionChanged(width_change));
  EXPECT_TRUE(layout_info.BoundsChanged(width_change));

  // 2. Changing only bottom represents a height change and therefore changes
  // the bounds.
  ItemLayoutInfo height_change = layout_info;
  height_change.bottom_ += 0.02f;
  EXPECT_FALSE(layout_info.PositionChanged(height_change));
  EXPECT_TRUE(layout_info.BoundsChanged(height_change));

  // 3. Edge differences below EPSILON are not treated as bounds changes.
  ItemLayoutInfo nearly_equal = layout_info;
  nearly_equal.right_ += 0.005f;
  nearly_equal.bottom_ += 0.005f;
  EXPECT_FALSE(layout_info.BoundsChanged(nearly_equal));
}

// Verifies that ItemAnimationRecord accumulates PRE, POST, and Changed flags.
TEST(ItemAnimationRecordTest, AccumulatesFlags) {
  ItemAnimationRecord record;

  // 1. A new record contains no flags.
  EXPECT_EQ(record.flags, ItemAnimationRecord::kNone);
  EXPECT_FALSE(record.HasFlag(ItemAnimationRecord::kPre));
  EXPECT_FALSE(record.HasFlag(ItemAnimationRecord::kPost));
  EXPECT_FALSE(record.HasFlag(ItemAnimationRecord::kChanged));

  // 2. Adding the same flag repeatedly is idempotent and does not affect other
  // flags.
  record.AddFlag(ItemAnimationRecord::kPre);
  record.AddFlag(ItemAnimationRecord::kPre);
  EXPECT_TRUE(record.HasFlag(ItemAnimationRecord::kPre));
  EXPECT_EQ(record.flags, static_cast<uint8_t>(ItemAnimationRecord::kPre));

  // 3. A record can store multiple distinct flags simultaneously.
  record.AddFlag(ItemAnimationRecord::kPost);
  record.AddFlag(ItemAnimationRecord::kChanged);
  EXPECT_TRUE(record.HasFlag(ItemAnimationRecord::kPre));
  EXPECT_TRUE(record.HasFlag(ItemAnimationRecord::kPost));
  EXPECT_TRUE(record.HasFlag(ItemAnimationRecord::kChanged));
}

}  // namespace
}  // namespace list
}  // namespace lynx
