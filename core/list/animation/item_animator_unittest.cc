// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/item_animator.h"

#include "core/list/testing/mock_animation_target.h"
#include "core/list/testing/mock_item_animator.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {
namespace {

// Verifies that ItemAnimator captures independent PRE and POST layout values.
TEST(ItemAnimatorTest, CapturesPreAndPostLayoutInfo) {
  MockItemAnimator item_animator;
  MockAnimationTarget target("item");

  // 1. Capture the target's complete logical bounds before the update.
  target.SetAnimationLayout(1, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info = item_animator.GetPreItemLayoutInfo(&target);

  EXPECT_FLOAT_EQ(pre_layout_info.left_, 10.f);
  EXPECT_FLOAT_EQ(pre_layout_info.top_, 20.f);
  EXPECT_FLOAT_EQ(pre_layout_info.right_, 40.f);
  EXPECT_FLOAT_EQ(pre_layout_info.bottom_, 60.f);

  // 2. Change the logical layout, then verify that POST captures the new
  // bounds without changing PRE.
  target.SetAnimationLayout(2, 100.f, 200.f, 50.f, 60.f);
  ItemLayoutInfo post_layout_info =
      item_animator.GetPostItemLayoutInfo(&target);

  EXPECT_FLOAT_EQ(post_layout_info.left_, 100.f);
  EXPECT_FLOAT_EQ(post_layout_info.top_, 200.f);
  EXPECT_FLOAT_EQ(post_layout_info.right_, 150.f);
  EXPECT_FLOAT_EQ(post_layout_info.bottom_, 260.f);
}

// Verifies the default durations and independent duration setters.
TEST(ItemAnimatorTest, StoresAnimationDurations) {
  MockItemAnimator item_animator;

  // 1. A new animator uses the List animation defaults.
  EXPECT_EQ(item_animator.add_duration_ms(), kDefaultAddAnimationDurationMs);
  EXPECT_EQ(item_animator.remove_duration_ms(),
            kDefaultRemoveAnimationDurationMs);
  EXPECT_EQ(item_animator.move_duration_ms(), kDefaultMoveAnimationDurationMs);
  EXPECT_EQ(item_animator.change_duration_ms(),
            kDefaultChangeAnimationDurationMs);

  // 2. Updating one animation type's duration does not overwrite the others.
  item_animator.SetAddDuration(10);
  item_animator.SetRemoveDuration(20);
  item_animator.SetMoveDuration(30);
  item_animator.SetChangeDuration(40);

  EXPECT_EQ(item_animator.add_duration_ms(), 10);
  EXPECT_EQ(item_animator.remove_duration_ms(), 20);
  EXPECT_EQ(item_animator.move_duration_ms(), 30);
  EXPECT_EQ(item_animator.change_duration_ms(), 40);
}

}  // namespace
}  // namespace list
}  // namespace lynx
