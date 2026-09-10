// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "base/include/auto_reset.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/animator_target.h"
#include "clay/gfx/animation/keyframe.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/value_animator.h"
#include "clay/gfx/geometry/transform_raw.h"
#include "clay/gfx/style/length.h"
#include "clay/public/value.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/testing/ui_test.h"
#include "gfx/animation/timing_function.h"
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
                            1,
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

class FixedSizeAnimatorTarget : public AnimatorTarget {
 public:
  explicit FixedSizeAnimatorTarget(FloatSize percentage_resolution_size,
                                   float opacity = 1.f)
      : percentage_resolution_size_(percentage_resolution_size),
        opacity_(opacity) {}

  FloatSize PercentageResolutionSize() override {
    return percentage_resolution_size_;
  }

  void GetProperty(ClayAnimationPropertyType type, float& value) override {
    if (type == ClayAnimationPropertyType::kOpacity) {
      value = opacity_;
    }
  }

  void SetProperty(ClayAnimationPropertyType type, float value,
                   bool skip_update_for_raster_animation) override {
    if (type == ClayAnimationPropertyType::kOpacity) {
      opacity_ = value;
    }
  }

  float opacity() const { return opacity_; }

 private:
  FloatSize percentage_resolution_size_;
  float opacity_;
};

class RecordingAnimationEventHandler : public AnimationEventHandler {
 public:
  void OnAnimationEvent(const AnimationParams& params) override {
    animation_events.push_back(params.event_type);
  }

  void OnTransitionEvent(const AnimationParams& params,
                         ClayAnimationPropertyType) override {
    transition_events.push_back(params.event_type);
  }

  std::vector<ClayEventType> animation_events;
  std::vector<ClayEventType> transition_events;
};

Value MakeCalcLengthValue(double percent, double fixed) {
  Value::Array calc;
  calc.emplace_back(percent);
  calc.emplace_back(static_cast<int32_t>(LengthUnit::kPercent));
  calc.emplace_back(fixed);
  calc.emplace_back(static_cast<int32_t>(LengthUnit::kNum));
  return Value(std::move(calc));
}

Value MakeTranslateXCalcValue(double percent, double fixed) {
  Value::Array op;
  op.emplace_back(static_cast<int32_t>(ClayTransformType::kTranslateX));
  op.emplace_back(MakeCalcLengthValue(percent, fixed));
  op.emplace_back(static_cast<int32_t>(LengthUnit::kCalc));
  op.emplace_back(0.0);
  op.emplace_back(static_cast<int32_t>(LengthUnit::kNum));
  op.emplace_back(0.0);
  op.emplace_back(static_cast<int32_t>(LengthUnit::kNum));

  Value::Array transform;
  transform.emplace_back(Value(std::move(op)));
  return Value(std::move(transform));
}
}  // namespace

TEST(TransformOperationsTest, TranslateYPercentageDetectionUsesFirstSlot) {
  TransformRaw op{};
  op.type = static_cast<int>(ClayTransformType::kTranslateY);
  op.values[0] = Length(1.f, LengthUnit::kPercent);

  auto keyframe_set = RawTransformKeyframeSet::Create();
  keyframe_set->AddKeyframe(RawTransformKeyframe::Create(
      fml::TimeDelta::Zero(), std::vector<TransformRaw>{op}));

  EXPECT_TRUE(keyframe_set->HasPercentageValues());
}

TEST(KeyframeSetTest, UsesGfxKeyframeTimingFunctionAfterClone) {
  auto keyframe_set =
      FloatKeyframeSet::Create(ClayAnimationPropertyType::kOpacity);
  auto start_keyframe = FloatKeyframe::Create(
      fml::TimeDelta::Zero(),
      lynx::gfx::StepsTimingFunction::Create(4, lynx::gfx::StepsType::kEnd));
  start_keyframe->SetValue(0.f);
  keyframe_set->AddKeyframe(std::move(start_keyframe));
  auto end_keyframe =
      FloatKeyframe::Create(fml::TimeDelta::FromSeconds(1),
                            lynx::gfx::LinearTimingFunction::Create());
  end_keyframe->SetValue(1.f);
  keyframe_set->AddKeyframe(std::move(end_keyframe));

  auto cloned_keyframe_set = keyframe_set->Clone(nullptr);
  auto* cloned_float_set =
      static_cast<FloatKeyframeSet*>(cloned_keyframe_set.get());
  EXPECT_FLOAT_EQ(cloned_float_set->GetValue(0.24f), 0.f);
  EXPECT_FLOAT_EQ(cloned_float_set->GetValue(0.26f), 0.25f);
}

TEST_F_UI(KeyFrameTest, TransformKeyframeCalcResolvesOnClone) {
  Value::Map start_properties;
  start_properties.emplace("transform", MakeTranslateXCalcValue(0.5, 10.0));
  Value::Map end_properties;
  end_properties.emplace("transform", MakeTranslateXCalcValue(0.5, 30.0));

  Value::Map keyframes;
  keyframes.emplace("0.000000", Value(std::move(start_properties)));
  keyframes.emplace("1.000000", Value(std::move(end_properties)));

  Value::Map keyframes_data;
  keyframes_data.emplace("transform_test", Value(std::move(keyframes)));
  page_->SetKeyframesData(Value(std::move(keyframes_data)));

  const KeyframesMap* keyframes_map = page_->GetKeyframesMap("transform_test");
  ASSERT_NE(keyframes_map, nullptr);
  auto it = keyframes_map->find(ClayAnimationPropertyType::kTransform);
  ASSERT_NE(it, keyframes_map->end());
  EXPECT_TRUE(it->second->HasPercentageValues());

  FixedSizeAnimatorTarget target(FloatSize(200.f, 100.f));
  KeyframesManager manager(&target);
  auto cloned_keyframe_set = it->second->Clone(&manager);
  auto* transform_set =
      static_cast<TransformKeyframeSet*>(cloned_keyframe_set.get());

  auto start_value = transform_set->GetValue(0.f);
  ASSERT_EQ(start_value.size(), 1u);
  EXPECT_FLOAT_EQ(start_value.GetOperations()[0].translate.x.value, 110.f);

  auto mid_value = transform_set->GetValue(0.5f);
  ASSERT_EQ(mid_value.size(), 1u);
  EXPECT_FLOAT_EQ(mid_value.GetOperations()[0].translate.x.value, 120.f);

  auto end_value = transform_set->GetValue(1.f);
  ASSERT_EQ(end_value.size(), 1u);
  EXPECT_FLOAT_EQ(end_value.GetOperations()[0].translate.x.value, 130.f);
}

TEST_F_UI(KeyFrameTest, TransitionStartsAfterNodeReady) {
  animator_view_->SetBound(0, 0, 100, 0);

  TransitionData height_transition;
  height_transition.property = ClayAnimationPropertyType::kHeight;
  height_transition.duration = 240;
  animator_view_->SetTransition({height_transition});

  animator_view_->SetBound(0, 0, 100, 200);

  EXPECT_FLOAT_EQ(animator_view_->Height(), 200.f);

  animator_view_->OnNodeReady();
  animator_view_->SetHeight(100);

  EXPECT_FLOAT_EQ(animator_view_->Height(), 200.f);
  EXPECT_TRUE(animator_view_->TransitionMgr()->IsAnimationRunning(
      ClayAnimationPropertyType::kHeight));
}

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

Value CreateAnimationEventKeyframes() {
  Value::Map move_start;
  move_start.emplace("transform", MakeTranslateXCalcValue(0.0, 0.0));
  Value::Map move_end;
  move_end.emplace("transform", MakeTranslateXCalcValue(0.0, 100.0));
  Value::Map move_keyframes;
  move_keyframes.emplace("0.000000", Value(std::move(move_start)));
  move_keyframes.emplace("1.000000", Value(std::move(move_end)));

  Value::Map color_keyframes;
  color_keyframes.emplace("0.000000",
                          Value{{"background-color", Value{0xffff0000u}}});
  color_keyframes.emplace("1.000000",
                          Value{{"background-color", Value{0xff0000ffu}}});

  Value::Map opacity_keyframes;
  opacity_keyframes.emplace("0.000000", Value{{"opacity", Value{0.1}}});
  opacity_keyframes.emplace("1.000000", Value{{"opacity", Value{1.0}}});

  Value::Map keyframes;
  keyframes.emplace("mymove", Value(std::move(move_keyframes)));
  keyframes.emplace("mycolor", Value(std::move(color_keyframes)));
  keyframes.emplace("myopacity", Value(std::move(opacity_keyframes)));
  return Value(std::move(keyframes));
}

Value CreateMixedRasterAndUiKeyframes() {
  Value::Array start_filter_op;
  start_filter_op.emplace_back(
      static_cast<int32_t>(ClayFilterType::kGrayScale));
  start_filter_op.emplace_back(0.0);
  Value::Array start_filter;
  start_filter.emplace_back(Value{std::move(start_filter_op)});
  Value::Map start;
  start.emplace("opacity", Value{0.1});
  start.emplace("filter", Value{std::move(start_filter)});

  Value::Array end_filter_op;
  end_filter_op.emplace_back(static_cast<int32_t>(ClayFilterType::kGrayScale));
  end_filter_op.emplace_back(1.0);
  Value::Array end_filter;
  end_filter.emplace_back(Value{std::move(end_filter_op)});
  Value::Map end;
  end.emplace("opacity", Value{1.0});
  end.emplace("filter", Value{std::move(end_filter)});

  Value::Map keyframes;
  keyframes.emplace("0.000000", Value(std::move(start)));
  keyframes.emplace("1.000000", Value(std::move(end)));
  return Value{{"mixed_test", Value(std::move(keyframes))}};
}

Value CreateMixedKeyframesWithMissingRasterEndpoints() {
  Value::Array start_filter_op;
  start_filter_op.emplace_back(
      static_cast<int32_t>(ClayFilterType::kGrayScale));
  start_filter_op.emplace_back(0.0);
  Value::Array start_filter;
  start_filter.emplace_back(Value{std::move(start_filter_op)});
  Value::Map start;
  start.emplace("filter", Value{std::move(start_filter)});

  Value::Map middle;
  middle.emplace("opacity", Value{0.8});

  Value::Array end_filter_op;
  end_filter_op.emplace_back(static_cast<int32_t>(ClayFilterType::kGrayScale));
  end_filter_op.emplace_back(1.0);
  Value::Array end_filter;
  end_filter.emplace_back(Value{std::move(end_filter_op)});
  Value::Map end;
  end.emplace("filter", Value{std::move(end_filter)});

  Value::Map keyframes;
  keyframes.emplace("0.000000", Value(std::move(start)));
  keyframes.emplace("0.500000", Value(std::move(middle)));
  keyframes.emplace("1.000000", Value(std::move(end)));
  return Value{{"mixed_missing_endpoints", Value(std::move(keyframes))}};
}

}  // namespace

TEST_F_UI(KeyFrameTest, SetBoundCouplesTopWithHeightTransition) {
  animator_view_->SetBound(0, 200, 100, 100);

  TransitionData height_transition;
  height_transition.property = ClayAnimationPropertyType::kHeight;
  height_transition.duration = 240;
  animator_view_->SetTransition({height_transition});

  animator_view_->OnNodeReady();
  animator_view_->SetBound(0, 100, 100, 200);

  EXPECT_FLOAT_EQ(animator_view_->GetBounds().x(), 0.f);
  EXPECT_FLOAT_EQ(animator_view_->GetBounds().y(), 200.f);
  EXPECT_FLOAT_EQ(animator_view_->GetBounds().width(), 100.f);
  EXPECT_FLOAT_EQ(animator_view_->GetBounds().height(), 100.f);
  EXPECT_TRUE(animator_view_->TransitionMgr()->IsAnimationRunning(
      ClayAnimationPropertyType::kHeight));
  EXPECT_TRUE(animator_view_->TransitionMgr()->IsAnimationRunning(
      ClayAnimationPropertyType::kTop));
}

// Test start and default values
TEST_F_UI(KeyFrameTest, DefaultStartAndEnd) {
  Value keyframes_data = CreateKeyFrameData1();

  page_->SetKeyframesData(keyframes_data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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
                            1,
                            ClayAnimationFillModeType::kBoth,
                            ClayAnimationDirectionType::kNormal,
                            ClayAnimationPlayStateType::kRunning};

  auto data = CreateKeyFrameData1();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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
  animator_view_->OnNodeReady();

  EXPECT_EQ(animator_view_->KeyframesMgr()
                ->animations()
                .front()
                .animator->start_time_,
            10000);
}

TEST_F_UI(KeyFrameTest, DuplicateSameNameAnimationDataKeepsLastEntry) {
  AnimationData duplicate_data{"opacity_test",
                               240,
                               0,
                               TimingFunctionData(),
                               1,
                               ClayAnimationFillModeType::kBoth,
                               ClayAnimationDirectionType::kNormal,
                               ClayAnimationPlayStateType::kRunning};

  page_->SetKeyframesData(CreateKeyFrameData1());
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animation_data_.push_back(duplicate_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

  ASSERT_EQ(animator_view_->KeyframesMgr()->animations().size(), 1u);
  EXPECT_EQ(animator_view_->KeyframesMgr()->animations().front().data.duration,
            duplicate_data.duration);
}

// Test fillmode forwards changed to backwards
TEST_F_UI(KeyFrameTest, ChangeFillmode) {
  AnimationData update_data{"opacity_test",
                            240,
                            0,
                            TimingFunctionData(),
                            1,
                            ClayAnimationFillModeType::kBackwards,
                            ClayAnimationDirectionType::kNormal,
                            ClayAnimationPlayStateType::kRunning};

  auto data = CreateKeyFrameData2();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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
  animator_view_->OnNodeReady();

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
                           1,
                           ClayAnimationFillModeType::kForwards,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kPaused};

  Value keyframes_data = CreateKeyFrameData1();

  page_->SetKeyframesData(keyframes_data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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
                           1,
                           ClayAnimationFillModeType::kForwards,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kPaused};

  auto data = CreateKeyFrameData3();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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
                           1,
                           ClayAnimationFillModeType::kBackwards,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kPaused};

  auto data = CreateKeyFrameData3();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.clear();
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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

TEST_F_UI(KeyFrameTest, BackwardsFillStartsAfterDelay) {
  page_->SetRasterAnimationEnabled(true);
  animator_view_->AddEventCallback(event_attr::kEventAnimationStart);

  std::vector<std::string> events;
  animation_event_callback_ = [&events](const std::string& event_name,
                                        const char* animation_name, int) {
    events.emplace_back(event_name + ":" + animation_name);
  };

  AnimationData start_data{"opacity_test",
                           240,
                           120,
                           TimingFunctionData(),
                           1,
                           ClayAnimationFillModeType::kBoth,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kRunning};
  page_->SetKeyframesData(CreateKeyFrameData3());
  animator_view_->SetBound(0, 0, 100, 100);
  animator_view_->SetAnimation({start_data});
  animator_view_->OnNodeReady();

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  animator->DoAnimationFrame(10000, false);
  EXPECT_FALSE(animator->IsRunning());
  EXPECT_TRUE(events.empty());

  animator->DoAnimationFrame(10119, false);
  EXPECT_FALSE(animator->IsRunning());
  EXPECT_TRUE(events.empty());

  animator->DoAnimationFrame(10120, false);
  EXPECT_TRUE(animator->IsRunning());
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test"));
}

// Test for animation start event
TEST_F_UI(KeyFrameTest, AnimationStartEvent) {
  AnimationData update_data1{"opacity_test",
                             240,
                             0,
                             TimingFunctionData(),
                             1,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};

  AnimationData update_data2{"opacity_test",
                             480,
                             0,
                             TimingFunctionData(),
                             1,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};
  MockAnimatorListener mock_listener;
  auto data = CreateKeyFrameData1();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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
  animator_view_->OnNodeReady();

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
  animator_view_->OnNodeReady();

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
                             1,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};

  AnimationData update_data2{"opacity_test",
                             480,
                             0,
                             TimingFunctionData(),
                             1,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};
  MockAnimatorListener mock_listener;
  auto data = CreateKeyFrameData1();

  page_->SetKeyframesData(data);
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.push_back(start_data_);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

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
  animator_view_->OnNodeReady();

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
  animator_view_->OnNodeReady();

  for (size_t i = 0; i < 20; i++) {
    animator_view_->KeyframesMgr()
        ->target_->GetAnimationHandler()
        ->DoAnimationFrame(frame_time);
    frame_time += 16;
  }

  animator->RemoveListener(&mock_listener);
}

TEST_F_UI(KeyFrameTest, RasterAnimationEventsRemainUIOwned) {
  page_->SetRasterAnimationEnabled(true);
  animator_view_->AddEventCallback(event_attr::kEventAnimationStart);
  animator_view_->AddEventCallback(event_attr::kEventAnimationEnd);

  std::vector<std::string> events;
  animation_event_callback_ = [&events](const std::string& event_name,
                                        const char* animation_name, int) {
    events.emplace_back(event_name + ":" + animation_name);
  };

  AnimationData initial_move{"mymove",
                             2000,
                             0,
                             TimingFunctionData(),
                             1,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};
  AnimationData move = initial_move;
  move.duration = 2300;
  AnimationData color = move;
  color.name = "mycolor";
  color.duration = 1000;
  AnimationData opacity = move;
  opacity.name = "myopacity";
  opacity.duration = 700;

  page_->SetKeyframesData(CreateAnimationEventKeyframes());
  animator_view_->SetBound(0, 0, 100, 100);
  animator_view_->SetAnimation({initial_move});
  animator_view_->OnNodeReady();
  page_->GetAnimationHandler()->DoAnimationFrame(10000);
  page_->GetAnimationHandler()->DoAnimationFrame(11000);
  animator_view_->SetAnimation({move, color, opacity});
  animator_view_->OnNodeReady();

  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:mymove",
                                             "animationstart:mycolor",
                                             "animationstart:myopacity"));

  RecordingAnimationEventHandler raster_events;
  FixedSizeAnimatorTarget transform_target(FloatSize(100.f, 100.f));
  FixedSizeAnimatorTarget color_target(FloatSize(100.f, 100.f));
  FixedSizeAnimatorTarget opacity_target(FloatSize(100.f, 100.f));
  auto transform_manager =
      animator_view_->KeyframesMgr()->CloneForRasterAnimation(
          ClayAnimationPropertyType::kTransform, &transform_target);
  auto color_manager = animator_view_->KeyframesMgr()->CloneForRasterAnimation(
      ClayAnimationPropertyType::kBackgroundColor, &color_target);
  auto opacity_manager =
      animator_view_->KeyframesMgr()->CloneForRasterAnimation(
          ClayAnimationPropertyType::kOpacity, &opacity_target);
  ASSERT_NE(transform_manager, nullptr);
  ASSERT_NE(color_manager, nullptr);
  ASSERT_NE(opacity_manager, nullptr);
  for (auto* manager :
       {transform_manager.get(), color_manager.get(), opacity_manager.get()}) {
    manager->SetEventHandler(&raster_events);
    manager->animations().front().animator->DoAnimationFrame(11000);
    EXPECT_TRUE(manager->animations().front().animator->StartListenersCalled());
  }
  EXPECT_TRUE(raster_events.animation_events.empty());

  FixedSizeAnimatorTarget replacement_target(FloatSize(100.f, 100.f));
  auto replacement_manager =
      animator_view_->KeyframesMgr()->CloneForRasterAnimation(
          ClayAnimationPropertyType::kOpacity, &replacement_target);
  ASSERT_NE(replacement_manager, nullptr);
  replacement_manager->SyncProperties(opacity_manager.get());
  ASSERT_EQ(replacement_manager->animations().size(), 1u);
  EXPECT_TRUE(replacement_manager->animations()
                  .front()
                  .animator->StartListenersCalled());
  replacement_manager->SetEventHandler(&raster_events);
  replacement_manager->animations().front().animator->DoAnimationFrame(11700);
  transform_manager->animations().front().animator->DoAnimationFrame(13300);
  color_manager->animations().front().animator->DoAnimationFrame(12000);
  EXPECT_TRUE(raster_events.animation_events.empty());

  page_->GetAnimationHandler()->DoAnimationFrame(11000);
  page_->GetAnimationHandler()->DoAnimationFrame(11700);
  page_->GetAnimationHandler()->DoAnimationFrame(12000);
  page_->GetAnimationHandler()->DoAnimationFrame(12300);

  EXPECT_THAT(events, ::testing::ElementsAre(
                          "animationstart:mymove", "animationstart:mycolor",
                          "animationstart:myopacity", "animationend:myopacity",
                          "animationend:mycolor", "animationend:mymove"));
}

TEST_F_UI(KeyFrameTest, PureRasterAnimationUsesLifecycleOnlyUiAnimator) {
  page_->SetRasterAnimationEnabled(true);
  page_->SetKeyframesData(CreateKeyFrameData3());
  animator_view_->SetBound(0, 0, 100, 100);
  animator_view_->SetAnimation({start_data_});
  animator_view_->OnNodeReady();

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  EXPECT_EQ(animator->GetFrameUpdateMode(),
            ValueAnimator::FrameUpdateMode::kLifecycleOnly);
  EXPECT_FLOAT_EQ(animator_view_->render_object()->Opacity(), 0.1f);

  EXPECT_FALSE(page_->GetAnimationHandler()->DoAnimationFrame(10000));
  EXPECT_FALSE(page_->GetAnimationHandler()->DoAnimationFrame(10080));
  EXPECT_FLOAT_EQ(animator_view_->render_object()->Opacity(), 0.1f);

  FixedSizeAnimatorTarget raster_target(FloatSize(100.f, 100.f));
  auto raster_manager = animator_view_->KeyframesMgr()->CloneForRasterAnimation(
      ClayAnimationPropertyType::kOpacity, &raster_target);
  ASSERT_NE(raster_manager, nullptr);
  ASSERT_EQ(raster_manager->animations().size(), 1u);
  EXPECT_EQ(raster_manager->animations().front().animator->GetFrameUpdateMode(),
            ValueAnimator::FrameUpdateMode::kUpdateValues);
}

TEST_F_UI(KeyFrameTest, MixedAnimationKeepsUiValueUpdates) {
  page_->SetRasterAnimationEnabled(true);
  page_->SetKeyframesData(CreateMixedRasterAndUiKeyframes());
  animator_view_->SetBound(0, 0, 100, 100);
  AnimationData data = start_data_;
  data.name = "mixed_test";
  animator_view_->SetAnimation({data});
  animator_view_->OnNodeReady();

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  EXPECT_EQ(animator->GetFrameUpdateMode(),
            ValueAnimator::FrameUpdateMode::kUpdateValues);
}

TEST_F_UI(KeyFrameTest, MixedRasterClonePreparesMissingEndpointsAfterDelay) {
  page_->SetRasterAnimationEnabled(true);
  page_->SetKeyframesData(CreateMixedKeyframesWithMissingRasterEndpoints());
  animator_view_->SetBound(0, 0, 100, 100);
  AnimationData data = start_data_;
  data.name = "mixed_missing_endpoints";
  data.delay = 80;
  data.fill_mode = ClayAnimationFillModeType::kNone;
  animator_view_->SetAnimation({data});
  animator_view_->OnNodeReady();

  ASSERT_FALSE(animator_view_->KeyframesMgr()
                   ->animations()
                   .front()
                   .animator->IsPreparedForPresentation());

  FixedSizeAnimatorTarget raster_target(FloatSize(100.f, 100.f), 0.4f);
  auto raster_manager = animator_view_->KeyframesMgr()->CloneForRasterAnimation(
      ClayAnimationPropertyType::kOpacity, &raster_target);
  ASSERT_NE(raster_manager, nullptr);
  ValueAnimator* raster_animator =
      raster_manager->animations().front().animator.get();

  raster_animator->DoAnimationFrame(0);
  raster_animator->DoAnimationFrame(80);
  EXPECT_FLOAT_EQ(raster_target.opacity(), 0.4f);
  raster_animator->DoAnimationFrame(160);
  EXPECT_FLOAT_EQ(raster_target.opacity(), 0.8f);
}

TEST_F_UI(KeyFrameTest, RasterTransitionEventsRemainUIOwned) {
  page_->SetRasterAnimationEnabled(true);
  animator_view_->AddEventCallback(event_attr::kEventTransitionStart);
  animator_view_->AddEventCallback(event_attr::kEventTransitionEnd);

  std::vector<std::string> events;
  transition_event_callback_ = [&events](const std::string& event_name,
                                         const char*, int,
                                         ClayAnimationPropertyType type) {
    events.emplace_back(event_name + ":" +
                        std::to_string(static_cast<int>(type)));
  };

  TransitionData transition;
  transition.property = ClayAnimationPropertyType::kOpacity;
  transition.duration = 160;
  animator_view_->SetTransition({transition});
  animator_view_->OnNodeReady();
  animator_view_->SetOpacity(1.f);

  const std::string opacity_type =
      std::to_string(static_cast<int>(ClayAnimationPropertyType::kOpacity));
  EXPECT_THAT(events,
              ::testing::ElementsAre("transitionstart:" + opacity_type));

  RecordingAnimationEventHandler raster_events;
  FixedSizeAnimatorTarget raster_target(FloatSize(100.f, 100.f));
  auto raster_manager =
      animator_view_->TransitionMgr()->CloneForRasterAnimation(
          ClayAnimationPropertyType::kOpacity, &raster_target);
  ASSERT_NE(raster_manager, nullptr);
  raster_manager->SetEventHandler(&raster_events);
  auto raster_animators = raster_manager->GetRunningAnimators();
  ASSERT_EQ(raster_animators.size(), 1u);
  raster_animators.front()->DoAnimationFrame(0);
  EXPECT_TRUE(raster_animators.front()->StartListenersCalled());
  EXPECT_TRUE(raster_events.transition_events.empty());

  FixedSizeAnimatorTarget replacement_target(FloatSize(100.f, 100.f));
  auto replacement_manager =
      animator_view_->TransitionMgr()->CloneForRasterAnimation(
          ClayAnimationPropertyType::kOpacity, &replacement_target);
  ASSERT_NE(replacement_manager, nullptr);
  replacement_manager->SyncProperties(raster_manager.get());
  replacement_manager->SetEventHandler(&raster_events);
  auto replacement_animators = replacement_manager->GetRunningAnimators();
  ASSERT_EQ(replacement_animators.size(), 1u);
  EXPECT_TRUE(replacement_animators.front()->StartListenersCalled());
  replacement_animators.front()->DoAnimationFrame(160);
  EXPECT_TRUE(raster_events.transition_events.empty());

  page_->GetAnimationHandler()->DoAnimationFrame(0);
  page_->GetAnimationHandler()->DoAnimationFrame(160);
  EXPECT_THAT(events, ::testing::ElementsAre("transitionstart:" + opacity_type,
                                             "transitionend:" + opacity_type));
}

TEST_F_UI(KeyFrameTest, RasterTransitionUsesLifecycleOnlyUiAnimator) {
  page_->SetRasterAnimationEnabled(true);

  TransitionData transition;
  transition.property = ClayAnimationPropertyType::kOpacity;
  transition.duration = 160;
  animator_view_->SetTransition({transition});
  animator_view_->OnNodeReady();
  animator_view_->SetOpacity(1.f);

  auto animators = animator_view_->TransitionMgr()->GetStartedAnimators();
  ASSERT_EQ(animators.size(), 1u);
  EXPECT_EQ(animators.front()->GetFrameUpdateMode(),
            ValueAnimator::FrameUpdateMode::kLifecycleOnly);

  EXPECT_FALSE(page_->GetAnimationHandler()->DoAnimationFrame(10000));
  EXPECT_FALSE(page_->GetAnimationHandler()->DoAnimationFrame(10080));
  EXPECT_FLOAT_EQ(animator_view_->render_object()->Opacity(), 0.1f);

  EXPECT_FALSE(page_->GetAnimationHandler()->DoAnimationFrame(10160));
  EXPECT_FLOAT_EQ(animator_view_->render_object()->Opacity(), 1.f);
}

TEST_F_UI(KeyFrameTest, RasterTransitionCanBeClonedDuringDelay) {
  page_->SetRasterAnimationEnabled(true);

  TransitionData transition;
  transition.property = ClayAnimationPropertyType::kOpacity;
  transition.duration = 160;
  transition.delay = 80;
  animator_view_->SetTransition({transition});
  animator_view_->OnNodeReady();
  animator_view_->SetOpacity(1.f);

  auto animators = animator_view_->TransitionMgr()->GetStartedAnimators();
  ASSERT_EQ(animators.size(), 1u);
  EXPECT_TRUE(animators.front()->IsStarted());
  EXPECT_FALSE(animators.front()->IsRunning());

  FixedSizeAnimatorTarget raster_target(FloatSize(100.f, 100.f));
  auto raster_manager =
      animator_view_->TransitionMgr()->CloneForRasterAnimation(
          ClayAnimationPropertyType::kOpacity, &raster_target);
  ASSERT_NE(raster_manager, nullptr);
  EXPECT_EQ(raster_manager->GetStartedAnimators().size(), 1u);
}

TEST_F_UI(KeyFrameTest, RasterTransitionRetargetsFromPresentationValue) {
  page_->SetRasterAnimationEnabled(true);

  TransitionData transition;
  transition.property = ClayAnimationPropertyType::kOpacity;
  transition.duration = 100000;
  animator_view_->SetTransition({transition});
  animator_view_->OnNodeReady();
  animator_view_->SetOpacity(1.f);

  const int64_t start_time =
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds() - 50000;
  page_->GetAnimationHandler()->DoAnimationFrame(start_time);
  auto animators = animator_view_->TransitionMgr()->GetStartedAnimators();
  ASSERT_EQ(animators.size(), 1u);
  // Freeze the presentation state so retargeting and the verification below
  // cannot observe different wall-clock samples.
  animators.front()->Pause();
  const float presentation_before_retarget = animator_view_->Opacity();
  EXPECT_GT(presentation_before_retarget, 0.1f);

  // Retarget to the unchanged UI-side underlying value. Before presentation
  // sampling, this equality caused SetOpacity() to return without interrupting
  // the raster transition.
  animator_view_->SetOpacity(0.1f);
  EXPECT_NEAR(animator_view_->Opacity(), presentation_before_retarget, 1e-5f);
}

TEST_F_UI(KeyFrameTest, HitTestUsesRasterTransformPresentationValue) {
  page_->SetRasterAnimationEnabled(true);

  animator_view_->SetBound(0, 0, 100, 100);
  TransitionData transition;
  transition.property = ClayAnimationPropertyType::kTransform;
  transition.duration = 100000;
  animator_view_->SetTransition({transition});
  animator_view_->OnNodeReady();

  lynx::gfx::TransformOperations target_transform;
  target_transform.AppendTranslate({100.0f, lynx::gfx::LengthUnit::kNumber}, {},
                                   {});
  animator_view_->SetTransform(target_transform, FloatPoint(0.f, 0.f));
  const int64_t start_time =
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds() - 50000;
  page_->GetAnimationHandler()->DoAnimationFrame(start_time);
  auto animators = animator_view_->TransitionMgr()->GetStartedAnimators();
  ASSERT_EQ(animators.size(), 1u);
  // GetPresentationTransform() and GetPointBySelf() sample independently.
  // Pause the animator so both calls use the same presentation state.
  animators.front()->Pause();

  FloatPoint visual_point(50.f, 50.f);
  animator_view_->GetPresentationTransform().TransformPoint(&visual_point);
  EXPECT_GT(visual_point.x(), 50.f);

  const FloatPoint local_point = animator_view_->GetPointBySelf(visual_point);
  EXPECT_NEAR(local_point.x(), 50.f, 1e-4f);
  EXPECT_NEAR(local_point.y(), 50.f, 1e-4f);
}

TEST_F_UI(KeyFrameTest, ShorterSameNameUpdateAfterEndDoesNotRestart) {
  AnimationData start_data{"opacity_test",
                           1000,
                           0,
                           TimingFunctionData(),
                           1,
                           ClayAnimationFillModeType::kNone,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kRunning};
  AnimationData update_data{"opacity_test",
                            500,
                            0,
                            TimingFunctionData(),
                            1,
                            ClayAnimationFillModeType::kBoth,
                            ClayAnimationDirectionType::kNormal,
                            ClayAnimationPlayStateType::kRunning};

  page_->SetKeyframesData(CreateKeyFrameData3());
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  animator->DoAnimationFrame(10000);
  animator->DoAnimationFrame(11000);

  ASSERT_FALSE(animator->IsStarted());

  MockAnimatorListener mock_listener;
  EXPECT_CALL(mock_listener, OnAnimationStart(::testing::_)).Times(0);
  EXPECT_CALL(mock_listener, OnAnimationEnd(::testing::_)).Times(0);
  animator->AddListener(&mock_listener);

  animation_data_.clear();
  animation_data_.push_back(update_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

  ASSERT_EQ(animator_view_->KeyframesMgr()->animations().front().animator.get(),
            animator);
  animator->DoAnimationFrame(11500);

  animator->RemoveListener(&mock_listener);
}

TEST_F_UI(KeyFrameTest,
          LifecycleOnlySameNameDurationExtensionReschedulesLifecycle) {
  page_->SetRasterAnimationEnabled(true);
  animator_view_->AddEventCallback(event_attr::kEventAnimationStart);
  animator_view_->AddEventCallback(event_attr::kEventAnimationEnd);

  const int64_t timeline_start_time =
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds() - 150;
  int64_t current_time = timeline_start_time;
  auto* handler = page_->GetAnimationHandler();
  std::vector<int64_t> scheduled_delays;
  handler->SetAnimationCallback([&scheduled_delays](int64_t delay) {
    scheduled_delays.push_back(delay);
  });
  struct CallbackCleanup {
    AnimationHandler* handler;
    ~CallbackCleanup() { handler->SetAnimationCallback(nullptr); }
  } callback_cleanup{handler};

  std::vector<std::string> events;
  lynx::base::AutoReset<decltype(animation_event_callback_)>
      event_callback_cleanup(
          &animation_event_callback_,
          [&events](const std::string& event_name, const char* animation_name,
                    int) {
            events.emplace_back(event_name + ":" + animation_name);
          });

  AnimationData initial_data{"opacity_test",
                             100,
                             0,
                             TimingFunctionData(),
                             1,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};
  page_->SetKeyframesData(CreateKeyFrameData3());
  animator_view_->SetBound(0, 0, 100, 100);
  animator_view_->SetAnimation({initial_data});
  animator_view_->OnNodeReady();

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  EXPECT_FALSE(handler->DoAnimationFrame(current_time));
  current_time = timeline_start_time + 100;
  EXPECT_FALSE(handler->DoAnimationFrame(current_time, true));
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test"));
  EXPECT_TRUE(animator->IsStarted());

  current_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  const size_t scheduled_count = scheduled_delays.size();
  AnimationData shorter_data = initial_data;
  shorter_data.duration = 50;
  animator_view_->SetAnimation({shorter_data});
  animator_view_->OnNodeReady();

  EXPECT_EQ(scheduled_delays.size(), scheduled_count);
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test"));
  EXPECT_TRUE(animator->IsStarted());
  EXPECT_EQ(handler->GetAnimationCount(), 1);
  EXPECT_FALSE(handler->DoAnimationFrame(current_time, true));
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test"));
  EXPECT_TRUE(animator->IsStarted());
  EXPECT_EQ(handler->GetAnimationCount(), 1);

  AnimationData longer_data = initial_data;
  longer_data.duration = current_time - timeline_start_time + 1000;
  animator_view_->SetAnimation({longer_data});
  animator_view_->OnNodeReady();

  ASSERT_EQ(animator_view_->KeyframesMgr()->animations().front().animator.get(),
            animator);
  ASSERT_FALSE(scheduled_delays.empty());
  EXPECT_EQ(scheduled_delays.back(), 0);
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test"));

  EXPECT_FALSE(handler->DoAnimationFrame(current_time, true));
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test",
                                             "animationstart:opacity_test"));
  current_time = timeline_start_time + longer_data.duration;
  EXPECT_FALSE(handler->DoAnimationFrame(current_time, true));
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test",
                                             "animationstart:opacity_test",
                                             "animationend:opacity_test"));
}

TEST_F_UI(KeyFrameTest,
          SameNamePausedFillRemovalDoesNotRestartRetainedAnimator) {
  page_->SetRasterAnimationEnabled(true);
  animator_view_->AddEventCallback(event_attr::kEventAnimationStart);
  animator_view_->AddEventCallback(event_attr::kEventAnimationEnd);

  const int64_t timeline_start_time =
      fml::TimePoint::Now().ToEpochDelta().ToMilliseconds() - 150;
  int64_t current_time = timeline_start_time;
  auto* handler = page_->GetAnimationHandler();

  std::vector<std::string> events;
  lynx::base::AutoReset<decltype(animation_event_callback_)>
      event_callback_cleanup(
          &animation_event_callback_,
          [&events](const std::string& event_name, const char* animation_name,
                    int) {
            events.emplace_back(event_name + ":" + animation_name);
          });

  AnimationData initial_data{"opacity_test",
                             100,
                             0,
                             TimingFunctionData(),
                             1,
                             ClayAnimationFillModeType::kBoth,
                             ClayAnimationDirectionType::kNormal,
                             ClayAnimationPlayStateType::kRunning};
  page_->SetKeyframesData(CreateKeyFrameData3());
  animator_view_->SetBound(0, 0, 100, 100);
  animator_view_->SetAnimation({initial_data});
  animator_view_->OnNodeReady();

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  EXPECT_FALSE(handler->DoAnimationFrame(current_time));
  current_time = timeline_start_time + 100;
  EXPECT_FALSE(handler->DoAnimationFrame(current_time, true));
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test"));
  EXPECT_TRUE(animator->IsStarted());
  EXPECT_EQ(handler->GetAnimationCount(), 1);

  current_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  AnimationData no_fill_data = initial_data;
  no_fill_data.duration = 50;
  no_fill_data.fill_mode = ClayAnimationFillModeType::kNone;
  no_fill_data.play_state = ClayAnimationPlayStateType::kPaused;
  animator_view_->SetAnimation({no_fill_data});
  animator_view_->OnNodeReady();

  ASSERT_EQ(animator_view_->KeyframesMgr()->animations().front().animator.get(),
            animator);
  EXPECT_THAT(events, ::testing::ElementsAre("animationstart:opacity_test",
                                             "animationend:opacity_test"));
  EXPECT_FALSE(animator->IsStarted());
  EXPECT_EQ(handler->GetAnimationCount(), 0);
}

TEST_F_UI(KeyFrameTest, SameNameUpdateDoesNotRestartAfterFrameTimeRollback) {
  AnimationData start_data{"opacity_test",
                           500,
                           0,
                           TimingFunctionData(),
                           1,
                           ClayAnimationFillModeType::kBoth,
                           ClayAnimationDirectionType::kNormal,
                           ClayAnimationPlayStateType::kRunning};
  AnimationData update_data{"opacity_test",
                            1500,
                            0,
                            TimingFunctionData(),
                            1,
                            ClayAnimationFillModeType::kBoth,
                            ClayAnimationDirectionType::kNormal,
                            ClayAnimationPlayStateType::kRunning};

  page_->SetKeyframesData(CreateKeyFrameData3());
  animator_view_->SetBound(0, 0, 100, 100);
  animation_data_.push_back(start_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

  ValueAnimator* animator =
      animator_view_->KeyframesMgr()->animations().front().animator.get();
  animator->DoAnimationFrame(10000);
  animator->DoAnimationFrame(10500);

  MockAnimatorListener mock_listener;
  EXPECT_CALL(mock_listener, OnAnimationStart(::testing::_)).Times(1);
  EXPECT_CALL(mock_listener, OnAnimationEnd(::testing::_)).Times(1);
  animator->AddListener(&mock_listener);

  animation_data_.clear();
  animation_data_.push_back(update_data);
  animator_view_->SetAnimation(animation_data_);
  animator_view_->OnNodeReady();

  ASSERT_EQ(animator_view_->KeyframesMgr()->animations().front().animator.get(),
            animator);
  animator->DoAnimationFrame(11000);
  animator->DoAnimationFrame(11500);
  animator->DoAnimationFrame(11490);

  animator->RemoveListener(&mock_listener);
}

}  // namespace clay
