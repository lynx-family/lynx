// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/animation/keyframe_effect.h"

#include <memory>

#include "core/animation/css_keyframe_manager.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/testing/mock_animation.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class KeyframeEffectTest : public ::testing::Test {
 public:
  KeyframeEffectTest() {}
  ~KeyframeEffectTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;

  fml::RefPtr<lynx::tasm::Element> element_;
  std::shared_ptr<animation::MockAnimation> animation_;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }

  animation::KeyframeEffect* InitTestEffect() {
    animation_ = std::make_shared<animation::MockAnimation>("test_animation");
    auto effect = animation::KeyframeEffect::Create();
    animation::KeyframeEffect* keyframe_effect = effect.get();
    InitTestEffectInternal(keyframe_effect);
    animation_->SetKeyframeEffect(std::move(effect));
    element_ = manager->CreateNode("view", nullptr);
    animation_->BindElement(element_.get());
    return keyframe_effect;
  }

  void InitTestEffectInternal(animation::KeyframeEffect* test_effect) {
    // add first model: color model
    std::unique_ptr<animation::KeyframedColorAnimationCurve> test_curve1(
        animation::KeyframedColorAnimationCurve::Create(
            starlight::XAnimationColorInterpolationType::kSRGB));
    auto test_frame1 =
        animation::ColorKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetColor(4294901760);
    test_curve1->AddKeyframe(std::move(test_frame1));
    test_curve1->type_ = animation::AnimationCurve::CurveType::OPACITY;
    auto test_frame2 = animation::ColorKeyframe::Create(
        fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame2->SetColor(4278255360);
    test_curve1->AddKeyframe(std::move(test_frame2));
    std::unique_ptr<animation::KeyframeModel> new_model1 =
        animation::KeyframeModel::Create(std::move(test_curve1));
    test_effect->AddKeyframeModel(std::move(new_model1));

    // add second model: layout model
    std::unique_ptr<animation::KeyframedLayoutAnimationCurve> test_curve2(
        animation::KeyframedLayoutAnimationCurve::Create());

    auto test_frame3 =
        animation::LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame3->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
    test_curve2->AddKeyframe(std::move(test_frame3));
    test_curve2->type_ = animation::AnimationCurve::CurveType::LEFT;
    auto test_frame4 = animation::LayoutKeyframe::Create(
        fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame4->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
    test_curve2->AddKeyframe(std::move(test_frame4));
    std::unique_ptr<animation::KeyframeModel> new_model2 =
        animation::KeyframeModel::Create(std::move(test_curve2));
    test_effect->AddKeyframeModel(std::move(new_model2));

    // add third model: opacity model
    std::unique_ptr<animation::KeyframedOpacityAnimationCurve> test_curve3(
        animation::KeyframedOpacityAnimationCurve::Create());

    auto test_frame5 =
        animation::OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame5->SetOpacity(1.0f);
    test_curve3->AddKeyframe(std::move(test_frame5));
    test_curve3->type_ = animation::AnimationCurve::CurveType::BGCOLOR;
    auto test_frame6 = animation::OpacityKeyframe::Create(
        fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame6->SetOpacity(0.0f);
    test_curve3->AddKeyframe(std::move(test_frame6));
    std::unique_ptr<animation::KeyframeModel> new_model3 =
        animation::KeyframeModel::Create(std::move(test_curve3));
    test_effect->AddKeyframeModel(std::move(new_model3));
  }

  starlight::AnimationData InitAnimationData(
      const base::String& name, long duration, long delay,
      starlight::TimingFunctionData timing_func, int iteration_count,
      starlight::AnimationFillModeType fill_mode,
      starlight::AnimationDirectionType direction,
      starlight::AnimationPlayStateType play_state) {
    starlight::AnimationData data;
    data.name = name;
    data.duration = duration;
    data.delay = delay;
    data.timing_func = timing_func;
    data.iteration_count = iteration_count;
    data.fill_mode = fill_mode;
    data.direction = direction;
    data.play_state = play_state;
    return data;
  }
};

TEST_F(KeyframeEffectTest, AddKeyframeModel) {
  animation::KeyframeEffect test_effect;
  EXPECT_TRUE(test_effect.keyframe_models().empty());

  // add first model: color model
  std::unique_ptr<animation::KeyframedColorAnimationCurve> test_curve1(
      animation::KeyframedColorAnimationCurve::Create(
          starlight::XAnimationColorInterpolationType::kSRGB));

  auto test_frame1 =
      animation::ColorKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetColor(4294901760);
  test_curve1->AddKeyframe(std::move(test_frame1));
  test_curve1->type_ = animation::AnimationCurve::CurveType::BGCOLOR;
  auto test_frame2 = animation::ColorKeyframe::Create(
      fml::TimeDelta::FromSecondsF(4.0), nullptr);
  test_frame2->SetColor(4278255360);
  test_curve1->AddKeyframe(std::move(test_frame2));

  std::unique_ptr<animation::KeyframeModel> new_model1 =
      animation::KeyframeModel::Create(std::move(test_curve1));
  test_effect.AddKeyframeModel(std::move(new_model1));

  EXPECT_EQ(1ul, test_effect.keyframe_models().size());

  // add second model: layout model
  std::unique_ptr<animation::KeyframedLayoutAnimationCurve> test_curve2(
      animation::KeyframedLayoutAnimationCurve::Create());

  auto test_frame3 =
      animation::LayoutKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame3->SetLayout(starlight::NLength::MakeUnitNLength(2.f));
  test_curve2->AddKeyframe(std::move(test_frame3));
  test_curve2->type_ = animation::AnimationCurve::CurveType::LEFT;
  auto test_frame4 = animation::LayoutKeyframe::Create(
      fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame4->SetLayout(starlight::NLength::MakeUnitNLength(4.f));
  test_curve2->AddKeyframe(std::move(test_frame4));

  std::unique_ptr<animation::KeyframeModel> new_model2 =
      animation::KeyframeModel::Create(std::move(test_curve2));
  test_effect.AddKeyframeModel(std::move(new_model2));

  EXPECT_EQ(2ul, test_effect.keyframe_models().size());

  // add third model: opacity model
  std::unique_ptr<animation::KeyframedOpacityAnimationCurve> test_curve3(
      animation::KeyframedOpacityAnimationCurve::Create());

  auto test_frame5 =
      animation::OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame5->SetOpacity(1.0f);
  test_curve3->AddKeyframe(std::move(test_frame5));
  test_curve3->type_ = animation::AnimationCurve::CurveType::OPACITY;
  auto test_frame6 = animation::OpacityKeyframe::Create(
      fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame6->SetOpacity(0.0f);
  test_curve3->AddKeyframe(std::move(test_frame6));

  std::unique_ptr<animation::KeyframeModel> new_model3 =
      animation::KeyframeModel::Create(std::move(test_curve3));
  test_effect.AddKeyframeModel(std::move(new_model3));

  EXPECT_EQ(3ul, test_effect.keyframe_models().size());
}

TEST_F(KeyframeEffectTest, SetStartTime) {
  animation::KeyframeEffect* test_effect = InitTestEffect();
  fml::TimePoint test_start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0));
  test_effect->SetStartTime(test_start_time);
  for (auto& model : test_effect->keyframe_models()) {
    EXPECT_EQ(model->start_time(), fml::TimePoint::FromEpochDelta(
                                       fml::TimeDelta::FromSecondsF(1.0)));
    EXPECT_EQ(model->GetRunState(),
              animation::KeyframeModel::RunState::STARTING);
  }
}

TEST_F(KeyframeEffectTest, SetPauseTime) {
  animation::KeyframeEffect* test_effect = InitTestEffect();
  fml::TimePoint test_pause_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0));
  test_effect->SetPauseTime(test_pause_time);
  for (auto& model : test_effect->keyframe_models()) {
    EXPECT_EQ(model->pause_time(), fml::TimePoint::FromEpochDelta(
                                       fml::TimeDelta::FromSecondsF(2.0)));
    EXPECT_EQ(model->GetRunState(), animation::KeyframeModel::RunState::PAUSED);
  }
}

TEST_F(KeyframeEffectTest, GetKeyframeModelByCurveType) {
  animation::KeyframeEffect* test_effect = InitTestEffect();
  animation::KeyframeModel* opacity_model =
      test_effect->GetKeyframeModelByCurveType(
          animation::AnimationCurve::CurveType::OPACITY);
  EXPECT_NE(nullptr, opacity_model);
}

TEST_F(KeyframeEffectTest, CheckHasFinished) {
  animation::KeyframeEffect* test_effect = InitTestEffect();

  std::unique_ptr<starlight::AnimationData> default_data =
      std::make_unique<starlight::AnimationData>();
  default_data->duration = 1000;
  default_data->delay = 0;
  default_data->iteration_count = 1;

  test_effect->UpdateAnimationData(default_data.get());
  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0));
  test_effect->SetStartTime(start_time);

  fml::TimePoint test_time1 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0));
  test_effect->TickKeyframeModel(test_time1);
  bool all_finished = test_effect->CheckHasFinished(test_time1);
  EXPECT_EQ(false, all_finished);

  fml::TimePoint test_time2 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.0));
  test_effect->TickKeyframeModel(test_time2);
  all_finished = test_effect->CheckHasFinished(test_time2);
  EXPECT_EQ(true, all_finished);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
