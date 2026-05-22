// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <memory>

#include "clay/common/element_id.h"
#include "clay/flow/animation/animation_mutator.h"
#include "clay/flow/animation/scroll_offset_animation.h"
#include "clay/flow/layers/transform_layer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(ScrollOffsetAnimation, Empty) {
  clay::ElementId element_id(1);
  float start_value = 0;
  auto animator = std::make_unique<clay::FlingAnimator>();
  animator->SetFriction(1.0f);
  animator->SetStartValue(start_value);
  animator->SetStartVelocity(5000);
  animator->FlingInitialize();
  // Bind animation to RenderScroll.
  auto scroll_offset_animation = std::make_shared<clay::ScrollOffsetAnimation>(
      0, clay::ScrollDirection::kVertical, std::move(animator));
  auto layer = std::make_shared<TransformLayer>(skity::Matrix::Translate(0, 0));
  auto mutator = AnimationMutator::Create(
      element_id, AnimationMutatorType::kScrollOffset, layer.get());
  mutator->asScrollOffset()->Initialize(skity::Vec2(0, 0), skity::Vec2(0, 0),
                                        skity::Rect::MakeEmpty(),
                                        skity::Rect::MakeEmpty());
  mutator->SetScrollOffsetAnimation(scroll_offset_animation);
  scroll_offset_animation->StartIfNeeded();
  int64_t now = scroll_offset_animation->GetAnimator()->GetLastAnimationTime();
  ASSERT_EQ(mutator->HasAnimationRunning(), true);
  ASSERT_EQ(mutator->DoAnimationFrame(now), true);
}

TEST(ScrollOffsetAnimation, ScrollAnimation) {
  clay::ElementId element_id(1);
  float start_value = 0;
  auto animator = std::make_unique<clay::FlingAnimator>();
  animator->SetFriction(1.0f);
  animator->SetStartValue(start_value);
  animator->SetStartVelocity(5000);
  animator->FlingInitialize();
  auto scroll_offset_animation = std::make_shared<clay::ScrollOffsetAnimation>(
      0, clay::ScrollDirection::kVertical, std::move(animator));
  auto layer = std::make_shared<TransformLayer>(skity::Matrix::Translate(0, 0));
  auto mutator = AnimationMutator::Create(
      element_id, AnimationMutatorType::kScrollOffset, layer.get());
  mutator->asScrollOffset()->Initialize(skity::Vec2(0, 0), skity::Vec2(0, 0),
                                        skity::Rect::MakeLTRB(0, 0, 600, 600),
                                        skity::Rect::MakeLTRB(0, 0, 600, 600));
  mutator->SetScrollOffsetAnimation(scroll_offset_animation);
  scroll_offset_animation->StartIfNeeded();
  int64_t now = scroll_offset_animation->GetAnimator()->GetLastAnimationTime();
  ASSERT_EQ(mutator->HasAnimationRunning(), true);
  ASSERT_EQ(mutator->DoAnimationFrame(now), false);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 100), false);
  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().y, -470);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 1000), true);
  ASSERT_EQ(mutator->HasAnimationRunning(), false);

  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().y, -600);
}

TEST(ScrollOffsetAnimation, ScrollAnimationHorizontal) {
  clay::ElementId element_id(1);
  float start_value = 0;
  auto animator = std::make_unique<clay::FlingAnimator>();
  animator->SetFriction(1.0f);
  animator->SetStartValue(start_value);
  animator->SetStartVelocity(5000);
  animator->FlingInitialize();
  auto scroll_offset_animation = std::make_shared<clay::ScrollOffsetAnimation>(
      0, clay::ScrollDirection::kHorizontal, std::move(animator));
  auto layer = std::make_shared<TransformLayer>(skity::Matrix::Translate(0, 0));
  auto mutator = AnimationMutator::Create(
      element_id, AnimationMutatorType::kScrollOffset, layer.get());
  mutator->asScrollOffset()->Initialize(skity::Vec2(0, 0), skity::Vec2(0, 0),
                                        skity::Rect::MakeLTRB(0, 0, 600, 600),
                                        skity::Rect::MakeLTRB(0, 0, 600, 600));
  mutator->SetScrollOffsetAnimation(scroll_offset_animation);
  scroll_offset_animation->StartIfNeeded();
  int64_t now = scroll_offset_animation->GetAnimator()->GetLastAnimationTime();
  ASSERT_EQ(mutator->HasAnimationRunning(), true);
  ASSERT_EQ(mutator->DoAnimationFrame(now), false);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 100), false);
  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().x, -470);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 1000), true);
  ASSERT_EQ(mutator->HasAnimationRunning(), false);

  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().x, -600);
}

TEST(ScrollOffsetAnimation, VisibleScrollOffset) {
  clay::ElementId element_id(1);
  float start_value = 0;
  auto animator = std::make_unique<clay::FlingAnimator>();
  animator->SetFriction(1.0f);
  animator->SetStartValue(start_value);
  animator->SetStartVelocity(5000);
  animator->FlingInitialize();
  auto scroll_offset_animation = std::make_shared<clay::ScrollOffsetAnimation>(
      0, clay::ScrollDirection::kVertical, std::move(animator));
  auto layer = std::make_shared<TransformLayer>(skity::Matrix::Translate(0, 0));
  auto mutator = AnimationMutator::Create(
      element_id, AnimationMutatorType::kScrollOffset, layer.get());
  mutator->asScrollOffset()->Initialize(skity::Vec2(0, 0), skity::Vec2(0, 0),
                                        skity::Rect::MakeLTRB(0, 0, 300, 300),
                                        skity::Rect::MakeLTRB(0, 0, 600, 600));
  mutator->SetScrollOffsetAnimation(scroll_offset_animation);
  scroll_offset_animation->StartIfNeeded();
  int64_t now = scroll_offset_animation->GetAnimator()->GetLastAnimationTime();
  ASSERT_EQ(mutator->HasAnimationRunning(), true);
  ASSERT_EQ(mutator->DoAnimationFrame(now), false);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 100), false);
  // Limited by visible offset range.
  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().y, -300);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 1000), true);
  ASSERT_EQ(mutator->HasAnimationRunning(), false);

  // Limited by visible offset range.
  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().y, -300);
}

TEST(ScrollOffsetAnimation, VisibleScrollOffsetHorizontal) {
  clay::ElementId element_id(1);
  float start_value = 0;
  auto animator = std::make_unique<clay::FlingAnimator>();
  animator->SetFriction(1.0f);
  animator->SetStartValue(start_value);
  animator->SetStartVelocity(5000);
  animator->FlingInitialize();
  auto scroll_offset_animation = std::make_shared<clay::ScrollOffsetAnimation>(
      0, clay::ScrollDirection::kHorizontal, std::move(animator));
  auto layer = std::make_shared<TransformLayer>(skity::Matrix::Translate(0, 0));
  auto mutator = AnimationMutator::Create(
      element_id, AnimationMutatorType::kScrollOffset, layer.get());
  mutator->asScrollOffset()->Initialize(skity::Vec2(0, 0), skity::Vec2(0, 0),
                                        skity::Rect::MakeLTRB(0, 0, 300, 300),
                                        skity::Rect::MakeLTRB(0, 0, 600, 600));
  mutator->SetScrollOffsetAnimation(scroll_offset_animation);
  scroll_offset_animation->StartIfNeeded();
  int64_t now = scroll_offset_animation->GetAnimator()->GetLastAnimationTime();
  ASSERT_EQ(mutator->HasAnimationRunning(), true);
  ASSERT_EQ(mutator->DoAnimationFrame(now), false);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 100), false);
  // Limited by visible offset range.
  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().x, -300);
  ASSERT_EQ(mutator->DoAnimationFrame(now + 1000), true);
  ASSERT_EQ(mutator->HasAnimationRunning(), false);

  // Limited by visible offset range.
  ASSERT_EQ(mutator->asScrollOffset()->GetScrollOffset().x, -300);
}

}  // namespace testing
}  // namespace clay
