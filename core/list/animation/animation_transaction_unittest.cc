// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#define private public
#include "core/list/animation/animation_transaction.h"
#undef private

#include "core/list/testing/mock_animation_target.h"
#include "core/list/testing/mock_item_animator.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {
namespace {

constexpr TransactionId kTransactionId = 7;

// AnimationTransaction has unique_ptr ownership members and is therefore
// non-copyable.
static_assert(!std::is_copy_constructible<AnimationTransaction>::value,
              "AnimationTransaction must retain exclusive ownership");
static_assert(!std::is_copy_assignable<AnimationTransaction>::value,
              "AnimationTransaction must retain exclusive ownership");

// Uses two live MockAnimationTargets to construct the PRE snapshot from before
// a data update.
TEST(AnimationTransactionTest, InitializesAndTransfersPreTargets) {
  // Create two live MockAnimationTargets.
  auto first_target = std::make_unique<MockAnimationTarget>("first");
  auto second_target = std::make_unique<MockAnimationTarget>("second");
  std::vector<WeakAnimationTarget> pre_targets{
      first_target->GetWeakAnimationTarget(),
      second_target->GetWeakAnimationTarget()};

  // 1. Let the transaction take exclusive ownership of ItemAnimator while
  // retaining the PRE target snapshot.
  auto item_animator = std::make_unique<MockItemAnimator>();
  ItemAnimator* item_animator_ptr = item_animator.get();
  AnimationTransaction transaction(kTransactionId, std::move(pre_targets),
                                   std::move(item_animator));

  // Verify that construction initializes transaction metadata, the animator,
  // and the recorder.
  EXPECT_EQ(transaction.id(), kTransactionId);
  EXPECT_EQ(transaction.state(), TransactionState::kHasPreChildrenSnapshot);
  EXPECT_EQ(&transaction.item_animator(), item_animator_ptr);
  EXPECT_TRUE(transaction.recorder().records_.empty());

  // 2. Destroy the first target to verify that pre_targets_ contains only weak
  // references and does not keep targets alive.
  first_target.reset();

  // The first Take preserves snapshot order: the destroyed target has expired,
  // while the live target remains accessible.
  std::vector<WeakAnimationTarget> taken_targets = transaction.TakePreTargets();
  ASSERT_EQ(taken_targets.size(), 2u);
  EXPECT_EQ(taken_targets[0].get(), nullptr);
  EXPECT_EQ(taken_targets[1].get(), second_target.get());

  // 3. The PRE snapshot is consumed only once. The first Take empties the
  // transaction's internal collection.
  EXPECT_TRUE(transaction.TakePreTargets().empty());
}

// Verifies that AnimationTransaction stores state without validating transition
// order.
TEST(AnimationTransactionTest, UpdatesState) {
  AnimationTransaction transaction(kTransactionId, {},
                                   std::make_unique<MockItemAnimator>());

  // 1. Store and read back kPreLayoutInfoRecorded.
  transaction.SetState(TransactionState::kPreLayoutInfoRecorded);
  EXPECT_EQ(transaction.state(), TransactionState::kPreLayoutInfoRecorded);

  // 2. Store and read back kRunning.
  transaction.SetState(TransactionState::kRunning);
  EXPECT_EQ(transaction.state(), TransactionState::kRunning);

  // 3. Store and read back kCancel.
  transaction.SetState(TransactionState::kCancel);
  EXPECT_EQ(transaction.state(), TransactionState::kCancel);
}

}  // namespace
}  // namespace list
}  // namespace lynx
