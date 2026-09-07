// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/item_animator_base.h"

#include "core/list/testing/mock_animation_target.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {
namespace {

class MockItemAnimatorBase : public ItemAnimatorBase {
 public:
  MOCK_METHOD(bool, AnimateRemoveImpl,
              (AnimationTarget*, const ItemLayoutInfo&), (override));
  MOCK_METHOD(bool, AnimateAddImpl, (AnimationTarget*, const ItemLayoutInfo&),
              (override));
  MOCK_METHOD(bool, AnimateMoveImpl,
              (AnimationTarget*, const ItemLayoutInfo&, const ItemLayoutInfo&),
              (override));
  MOCK_METHOD(bool, AnimateChangeImpl,
              (AnimationTarget*, const ItemLayoutInfo&, const ItemLayoutInfo&),
              (override));

  void RunPendingAnimations() override {}
  bool HasInvalidatedRunningAnimationLayout() const override { return false; }
  void CancelAnimations(bool) override {}
};

// Verifies that ItemAnimatorBase delegates each high-level operation to the
// corresponding primitive implementation.
TEST(ItemAnimatorBaseTest, DelegatesAnimationOperationsToImplementations) {
  MockItemAnimatorBase item_animator;
  MockAnimationTarget target("item");
  ItemLayoutInfo pre_layout_info;
  ItemLayoutInfo post_layout_info;

  // 1. Disappearance delegates to Remove and propagates true.
  EXPECT_CALL(item_animator,
              AnimateRemoveImpl(&target, testing::Ref(pre_layout_info)))
      .WillOnce(testing::Return(true));
  EXPECT_TRUE(item_animator.AnimateDisappearance(&target, pre_layout_info));

  // 2. Appearance passes the target and POST snapshot to Add and propagates
  // false.
  EXPECT_CALL(item_animator,
              AnimateAddImpl(&target, testing::Ref(post_layout_info)))
      .WillOnce(testing::Return(false));
  EXPECT_FALSE(item_animator.AnimateAppearance(&target, post_layout_info));

  // 3. Persistence passes one target and its PRE/POST snapshots to Move.
  EXPECT_CALL(item_animator,
              AnimateMoveImpl(&target, testing::Ref(pre_layout_info),
                              testing::Ref(post_layout_info)))
      .WillOnce(testing::Return(true));
  EXPECT_TRUE(item_animator.AnimatePersistence(&target, pre_layout_info,
                                               post_layout_info));

  // 4. Change passes one target and its PRE/POST snapshots to Change and
  // propagates false.
  EXPECT_CALL(item_animator,
              AnimateChangeImpl(&target, testing::Ref(pre_layout_info),
                                testing::Ref(post_layout_info)))
      .WillOnce(testing::Return(false));
  EXPECT_FALSE(
      item_animator.AnimateChange(&target, pre_layout_info, post_layout_info));
}

}  // namespace
}  // namespace list
}  // namespace lynx
