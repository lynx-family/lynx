// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/keyframe.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/value_animator.h"
#include "clay/public/value.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

class KeyFrameTest : public UITest {
 protected:
  void UISetUp() override {
    animator_view_ = std::make_unique<BaseView>(
        -1, "test_view", std::make_unique<RenderContainer>(), page_.get());
    animator_view_->SetAttribute("opacity", clay::Value(0.1));
    page_->AddChild(animator_view_.get());
    page_->SetRasterAnimationEnabled(false);
  }

  void UITearDown() override {
    animator_view_ = nullptr;
    animation_data_.clear();
  }

  std::unique_ptr<BaseView> animator_view_;
  std::vector<AnimationData> animation_data_;
  AnimationData start_data_{"opacity_test",
                            160,
                            0,
                            TimingFunctionData(),
                            0,
                            ClayAnimationFillModeType::kBoth,
                            ClayAnimationDirectionType::kNormal,
                            ClayAnimationPlayStateType::kRunning};
};

namespace {
class MockAnimatorListener : public AnimatorListener {
 public:
  // NOLINTNEXTLINE
  MOCK_METHOD(void, OnAnimationStart, (Animator & animation), (override));
  // NOLINTNEXTLINE
  MOCK_METHOD(void, OnAnimationCancel, (Animator & animation), (override));
  // NOLINTNEXTLINE
  MOCK_METHOD(void, OnAnimationEnd, (Animator & animation), (override));
  // NOLINTNEXTLINE
  MOCK_METHOD(void, OnAnimationRepeat, (Animator & animation), (override));
};
}  // namespace

namespace {

static std::string FractionKeyFromPercent(int percent) {
  return std::to_string(static_cast<double>(percent) * 0.01);
}

static Value MakeOpacityTestKeyframes(
    std::initializer_list<std::pair<int, double>> percent_to_opacity) {
  Value::Map keyframes;
  for (const auto& item : percent_to_opacity) {
    const int percent = item.first;
    const double opacity = item.second;
    keyframes.emplace(FractionKeyFromPercent(percent),
                      Value{{"opacity", Value{opacity}}});
  }

  return Value{{"opacity_test", Value{std::move(keyframes)}}};
}

Value CreateKeyFrameData1() {
  return MakeOpacityTestKeyframes({{50, 0.3}, {90, 1.0}});
}

Value CreateKeyFrameData2() {
  return MakeOpacityTestKeyframes({{50, 0.3}, {100, 1.0}});
}

Value CreateKeyFrameData3() {
  return MakeOpacityTestKeyframes({{0, 0.3}, {100, 1.0}});
}

}  // namespace

// Test start and default values
TEST_F_UI(KeyFrameTest, DefaultStartAndEnd) {
  Value keyframes_data = CreateKeyFrameData1();

  page_->SetKeyframesData(keyframes_data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i < 10; i++) {
    animator_view_->GetAnimationHandler()->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  EXPECT_TRUE(
      animator_view_->KeyframesMgr()->animations().front().keyframes_map.find(
          ClayAnimationPropertyType::kOpacity) !=
      animator_view_->KeyframesMgr()->animations().front().keyframes_map.end());
  FloatKeyframeSet* keyframe_sets = static_cast<FloatKeyframeSet*>(
      animator_view_->KeyframesMgr()
          ->animations_.front()
          .keyframes_map[ClayAnimationPropertyType::kOpacity]
          .get());

  EXPECT_EQ(keyframe_sets->keyframes_.front()->Value(), 0.1f);
  EXPECT_EQ(keyframe_sets->keyframes_.back()->Value(), 0.1f);
}

TEST_F_UI(KeyFrameTest, StartAnimationOnNodeReadyAfterKeyframesReady) {
  Value keyframes_data = CreateKeyFrameData1();

  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);

  EXPECT_TRUE(animator_view_->KeyframesMgr()->animations().empty());

  page_->SetKeyframesData(keyframes_data);
  animator_view_->OnNodeReady();

  ASSERT_EQ(animator_view_->KeyframesMgr()->animations().size(), 1u);
  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  EXPECT_TRUE(animator->StartListenersCalled());

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i < 10; i++) {
    animator_view_->GetAnimationHandler()->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  EXPECT_GT(animator_view_->render_object()->Opacity(), 0.1f);
}

// Test update animation properties
TEST_F_UI(KeyFrameTest, UpdateAnimation) {
  AnimationData update_data{"opacity_test",
                            240,
                            0,
                            TimingFunctionData(),
                            0,
                            ClayAnimationFillModeType::kBoth,
                            ClayAnimationDirectionType::kNormal,
                            ClayAnimationPlayStateType::kRunning};

  auto data = CreateKeyFrameData1();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);

  int64_t frame_time = 10000;
  for (size_t i = 0; i < 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }
  // update animation
  animation_data_.clear();
  animation_data_.push_back(update_data);
  animator_view_->SetAnimation(animation_data_);

  EXPECT_EQ(animator_view_->KeyframesMgr()
                ->animations()
                .front()
                .animator->start_time_,
            10000);
}

// Test fillmode forwards changed to backwards
TEST_F_UI(KeyFrameTest, ChangeFillmode) {
  AnimationData update_data{"opacity_test",
                            240,
                            0,
                            TimingFunctionData(),
                            0,
                            ClayAnimationFillModeType::kBackwards,
                            ClayAnimationDirectionType::kNormal,
                            ClayAnimationPlayStateType::kRunning};

  auto data = CreateKeyFrameData2();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i <= 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  EXPECT_EQ(animator_view_->render_object()->Opacity(), 1.f);
  // update animation
  animation_data_.clear();
  animation_data_.push_back(update_data);
  animator_view_->SetAnimation(animation_data_);

  for (size_t i = 0; i < 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  EXPECT_EQ(animator_view_->render_object()->Opacity(), 0.1f);
}

// Test animation delay attribute
TEST_F_UI(KeyFrameTest, AnimationDelay) {
  // Test animation delay less than 0.
  // With play_state paused, the animation should be frozen at the seeked
  // position implied by the negative delay.
  AnimationData start_data{"opacity_test",
                           240,
                           -120,
                           TimingFunctionData(),
                           0,
                           ClayAnimationFillModeType::kForwards,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kPaused};

  Value keyframes_data = CreateKeyFrameData1();

  page_->SetKeyframesData(keyframes_data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i < 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  EXPECT_EQ(animator_view_->render_object()->Opacity(), 0.3f);
}

// Test animation delay more than 0 and fillmode is forwards
TEST_F_UI(KeyFrameTest, AnimationDelayCombineForwards) {
  AnimationData start_data{"opacity_test",
                           240,
                           120,
                           TimingFunctionData(),
                           0,
                           ClayAnimationFillModeType::kForwards,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kPaused};

  auto data = CreateKeyFrameData3();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i < 7; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  animator_view_->KeyframesMgr()
      ->target_->GetAnimationHandler()
      ->DoAnimationFrame(frame_time);

  EXPECT_EQ(animator_view_->render_object()->Opacity(), 0.1f);
}

// Test animation delay more than 0 and fillmode is backwards
TEST_F_UI(KeyFrameTest, AnimationDelayCombineBackwards) {
  AnimationData start_data{"opacity_test",
                           240,
                           120,
                           TimingFunctionData(),
                           0,
                           ClayAnimationFillModeType::kBackwards,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kPaused};

  auto data = CreateKeyFrameData3();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i < 7; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  animator_view_->KeyframesMgr()
      ->target_->GetAnimationHandler()
      ->DoAnimationFrame(frame_time);

  EXPECT_EQ(animator_view_->render_object()->Opacity(), 0.3f);
}

// Test for animation start event
TEST_F_UI(KeyFrameTest, AnimationStartEvent) {
  AnimationData update_data1{"opacity_test",
                             240,
                             0,
                             TimingFunctionData(),
                             0,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};

  AnimationData update_data2{"opacity_test",
                             480,
                             0,
                             TimingFunctionData(),
                             0,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};
  MockAnimatorListener mock_listener;
  auto data = CreateKeyFrameData1();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations_.front().animator.get();
  EXPECT_EQ(animator->StartListenersCalled(), true);

  EXPECT_CALL(mock_listener, OnAnimationStart(::testing::_)).Times(1);
  animator->AddListener(&mock_listener);

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i < 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  // update animation
  animation_data_.clear();
  animation_data_.push_back(update_data1);
  animator_view_->SetAnimation(animation_data_);

  for (size_t i = 0; i < 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  // update animation
  animation_data_.clear();
  animation_data_.push_back(update_data2);
  animator_view_->SetAnimation(animation_data_);

  for (size_t i = 0; i < 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  EXPECT_EQ(animator->StartListenersCalled(), true);

  animator->RemoveListener(&mock_listener);
}

// Test for animation end event
TEST_F_UI(KeyFrameTest, AnimationEndEvent) {
  AnimationData update_data1{"opacity_test",
                             240,
                             0,
                             TimingFunctionData(),
                             0,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};

  AnimationData update_data2{"opacity_test",
                             480,
                             0,
                             TimingFunctionData(),
                             0,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};
  MockAnimatorListener mock_listener;
  auto data = CreateKeyFrameData1();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations_.front().animator.get();
  EXPECT_EQ(animator->StartListenersCalled(), true);

  EXPECT_CALL(mock_listener, OnAnimationEnd(::testing::_)).Times(2);
  animator->AddListener(&mock_listener);

  int64_t frame_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  for (size_t i = 0; i < 9; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  // update animation
  animation_data_.clear();
  animation_data_.push_back(update_data1);

  animator_view_->SetAnimation(animation_data_);

  for (size_t i = 0; i < 10; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  // update animation
  animation_data_.clear();
  animation_data_.push_back(update_data2);

  animator_view_->SetAnimation(animation_data_);

  for (size_t i = 0; i < 20; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  animator->RemoveListener(&mock_listener);
}

}  // namespace clay
