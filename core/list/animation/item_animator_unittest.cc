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

// Verify that stages are stored by value and remain independent of caller
// modifications.
TEST(ItemAnimatorTest, StoresIndependentAnimationStages) {
  MockItemAnimator item_animator;
  auto stages = MakeDefaultAnimationStages(20, 30, 10, 40);
  const auto expected = stages;

  // 1. Preserve stage order, animation types, and durations when storing a
  // config.
  item_animator.SetAnimationStages(stages);
  EXPECT_EQ(item_animator.animation_stages(), expected);

  // 2. Modifying the input stages leaves the animator's config snapshot
  // unchanged.
  stages.front().front().duration_ms = 999;
  stages.pop_back();
  EXPECT_EQ(item_animator.animation_stages(), expected);

  // 3. Setting a new config replaces all previously stored stages.
  item_animator.SetAnimationStages(stages);
  EXPECT_EQ(item_animator.animation_stages(), stages);
}

}  // namespace
}  // namespace list
}  // namespace lynx
