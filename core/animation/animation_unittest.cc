// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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

class AnimationTest : public ::testing::Test {
 public:
  AnimationTest() {}
  ~AnimationTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  fml::RefPtr<lynx::tasm::Element> element_;

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

  std::shared_ptr<animation::MockAnimation> InitTestAnimation() {
    auto test_animation =
        std::make_shared<animation::MockAnimation>("test_animation");
    auto effect = animation::KeyframeEffect::Create();
    InitTestEffect(*effect);
    test_animation->SetKeyframeEffect(std::move(effect));
    element_ = manager->CreateNode("view", nullptr);
    test_animation->BindElement(element_.get());
    return test_animation;
  }

  void InitTestEffect(animation::KeyframeEffect& test_effect) {
    std::unique_ptr<animation::KeyframedOpacityAnimationCurve> test_curve(
        animation::KeyframedOpacityAnimationCurve::Create());

    // keyframe 0% opacity 1
    auto test_frame1 =
        animation::OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetOpacity(0.0f);
    test_curve->AddKeyframe(std::move(test_frame1));
    test_curve->type_ = animation::AnimationCurve::CurveType::OPACITY;
    // keyframe 100% opacity 0
    auto test_frame2 = animation::OpacityKeyframe::Create(
        fml::TimeDelta::FromSecondsF(1.0), nullptr);
    test_frame2->SetOpacity(1.0f);
    test_curve->AddKeyframe(std::move(test_frame2));
    std::unique_ptr<animation::KeyframeModel> new_model =
        animation::KeyframeModel::Create(std::move(test_curve));
    test_effect.AddKeyframeModel(std::move(new_model));
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

TEST_F(AnimationTest, Pause) {
  auto test_animation = InitTestAnimation();
  test_animation->Pause();
  EXPECT_TRUE(test_animation->GetState() ==
              animation::Animation::State::kPause);
}

TEST_F(AnimationTest, Stop) {
  auto test_animation = InitTestAnimation();
  test_animation->Stop();
  EXPECT_TRUE(test_animation->GetState() == animation::Animation::State::kStop);
}

TEST_F(AnimationTest, SetKeyframeEffect) {
  auto test_animation = InitTestAnimation();
  auto test_effect = animation::KeyframeEffect::Create();
  InitTestEffect(*test_effect.get());
  auto temp_effect = test_effect.get();
  test_animation->SetKeyframeEffect(std::move(test_effect));
  EXPECT_TRUE(temp_effect->GetAnimation() == test_animation.get());
  EXPECT_TRUE(test_animation->keyframe_effect() == temp_effect);
}

TEST_F(AnimationTest, HasFinishedAll) {
  {
    auto test_animation = InitTestAnimation();
    auto test_animation_data =
        InitAnimationData(base::String("test_animation"), 3000, 2000,
                          starlight::TimingFunctionData(), 1,
                          starlight::AnimationFillModeType::kBoth,
                          starlight::AnimationDirectionType::kNormal,
                          starlight::AnimationPlayStateType::kRunning);

    test_animation->UpdateAnimationData(test_animation_data);
    fml::TimePoint test_time0 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0));
    test_animation->DoFrame(test_time0);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time0) == false);

    fml::TimePoint test_time1 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0));
    test_animation->DoFrame(test_time1);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time1) == false);

    test_time1 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.1));
    test_animation->DoFrame(test_time1);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time1) == false);

    fml::TimePoint test_time2 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(6.1));
    test_animation->DoFrame(test_time2);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time2) == true);
  }

  {
    auto test_animation = InitTestAnimation();
    auto test_animation_data =
        InitAnimationData(base::String("test_animation"), 1000, 0,
                          starlight::TimingFunctionData(), 3,
                          starlight::AnimationFillModeType::kBoth,
                          starlight::AnimationDirectionType::kNormal,
                          starlight::AnimationPlayStateType::kRunning);
    test_animation->UpdateAnimationData(test_animation_data);
    // update start time
    fml::TimePoint start_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));
    fml::TimePoint test_time0 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.1f));
    test_animation->DoFrame(start_time);
    test_animation->DoFrame(test_time0);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time0) == false);

    test_time0 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0));
    test_animation->DoFrame(test_time0);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time0) == false);

    test_time0 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.0));
    test_animation->DoFrame(test_time0);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time0) == false);

    test_time0 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.9));
    test_animation->DoFrame(test_time0);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time0) == false);

    test_time0 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.1));
    test_animation->DoFrame(test_time0);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time0) == true);

    test_time0 =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.0));
    test_animation->DoFrame(test_time0);
    EXPECT_TRUE(test_animation->HasFinishedAll(test_time0) == true);
  }
}

TEST_F(AnimationTest, UpdateAnimationData) {
  auto test_animation = InitTestAnimation();
  auto test_effect = animation::KeyframeEffect::Create();
  InitTestEffect(*test_effect.get());
  auto test_animation_data = starlight::AnimationData();
  test_animation->UpdateAnimationData(test_animation_data);
  EXPECT_TRUE(*(test_animation->animation_data()) == test_animation_data);
}

TEST_F(AnimationTest, CheckStartTime) {
  auto test_animation = InitTestAnimation();
  auto test_effect = animation::KeyframeEffect::Create();
  InitTestEffect(*test_effect.get());
  test_animation->SetKeyframeEffect(std::move(test_effect));
  auto test_animation_data = starlight::AnimationData();
  test_animation_data.name = "test_animation";
  test_animation->UpdateAnimationData(test_animation_data);

  // check start time
  test_animation->Play();
  EXPECT_TRUE(test_animation->start_time() ==
              animation::Animation::GetAnimationDummyStartTime());
  EXPECT_TRUE(
      test_animation->keyframe_effect()->keyframe_models()[0]->start_time() ==
      animation::Animation::GetAnimationDummyStartTime());
  fml::TimePoint tick_time = fml::TimePoint::FromTicks(100);
  test_animation->DoFrame(tick_time);
  EXPECT_TRUE(test_animation->start_time() == tick_time);
  EXPECT_TRUE(
      test_animation->keyframe_effect()->keyframe_models()[0]->start_time() ==
      tick_time);
}

TEST_F(AnimationTest, SendStartEvent) {
  auto test_animation = InitTestAnimation();
  auto test_animation_data = starlight::AnimationData();
  test_animation_data.name = "test_animation";
  test_animation_data.duration = 1000;
  test_animation->UpdateAnimationData(test_animation_data);

  // keyframe test
  test_animation->SendStartEvent();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "animationstart") == 0);
  lepus::Value params = tasm_mediator->GetAnimationEventParams();
  EXPECT_TRUE(params.IsTable());
  EXPECT_TRUE(params.Table()->Contains("new_animator") &&
              params.Table()->GetValue("new_animator").Bool());
  EXPECT_TRUE(params.Table()->Contains("animation_type") &&
              params.Table()
                  ->GetValue("animation_type")
                  .String()
                  .IsEqual("keyframe-animation"));
  EXPECT_TRUE(params.Table()->Contains("animation_name") &&
              params.Table()
                  ->GetValue("animation_name")
                  .String()
                  .IsEqual("test_animation"));

  test_animation->Play();
  test_animation->Destroy();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "animationcancel") == 0);

  // transition test
  test_animation = InitTestAnimation();
  test_animation->SetTransitionFlag();
  test_animation->UpdateAnimationData(test_animation_data);

  test_animation->SendStartEvent();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "transitionstart") == 0);
  params = tasm_mediator->GetAnimationEventParams();
  EXPECT_TRUE(params.IsTable());
  EXPECT_TRUE(params.Table()->Contains("new_animator") &&
              params.Table()->GetValue("new_animator").Bool());
  EXPECT_TRUE(params.Table()->Contains("animation_type") &&
              params.Table()
                  ->GetValue("animation_type")
                  .String()
                  .IsEqual("transition-animation"));
  EXPECT_TRUE(params.Table()->Contains("animation_name") &&
              params.Table()
                  ->GetValue("animation_name")
                  .String()
                  .IsEqual("test_animation"));

  test_animation->Play();
  test_animation->Destroy();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "transitioncancel") == 0);
}

TEST_F(AnimationTest, SendEndEvent) {
  auto test_animation = InitTestAnimation();
  auto test_animation_data = starlight::AnimationData();
  test_animation_data.name = "test_animation";
  test_animation->UpdateAnimationData(test_animation_data);

  // keyframe test
  test_animation->SendEndEvent();
  EXPECT_TRUE(
      std::strcmp(tasm_mediator->GetAnimationEventType(), "animationend") == 0);

  // transition test
  test_animation = InitTestAnimation();
  test_animation->SetTransitionFlag();
  test_animation->UpdateAnimationData(test_animation_data);

  test_animation->SendEndEvent();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "transitionend") == 0);
}

TEST_F(AnimationTest, SendCancelEvent) {
  auto test_animation = InitTestAnimation();
  auto test_animation_data = starlight::AnimationData();
  test_animation_data.name = "test_animation";
  test_animation->UpdateAnimationData(test_animation_data);

  // keyframe test
  test_animation->SendCancelEvent();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "animationcancel") == 0);

  // transition test
  test_animation = InitTestAnimation();
  test_animation->SetTransitionFlag();
  test_animation->UpdateAnimationData(test_animation_data);

  test_animation->SendCancelEvent();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "transitioncancel") == 0);
}

TEST_F(AnimationTest, SendIterationEvent) {
  auto test_animation = InitTestAnimation();
  auto test_animation_data = starlight::AnimationData();
  test_animation_data.name = "test_animation";
  test_animation->UpdateAnimationData(test_animation_data);

  // keyframe test
  test_animation->SendIterationEvent();
  EXPECT_TRUE(std::strcmp(tasm_mediator->GetAnimationEventType(),
                          "animationiteration") == 0);
}

TEST_F(AnimationTest, TestDelayEvent) {
  // fill mode forwards
  {
    auto test_animation = InitTestAnimation();
    auto css_keyframe_manager = std::make_unique<animation::CSSKeyframeManager>(
        test_animation->GetElement());
    test_animation->BindDelegate(css_keyframe_manager.get());
    test_animation->keyframe_effect()->BindAnimationDelegate(
        css_keyframe_manager.get());
    std::unique_ptr<starlight::AnimationData> animation_data =
        std::make_unique<starlight::AnimationData>();
    animation_data->duration = 1000;
    animation_data->iteration_count = 1;
    animation_data->delay = 1000;
    animation_data->fill_mode = starlight::AnimationFillModeType::kForwards;
    test_animation->UpdateAnimationData(*animation_data);
    // update start time
    fml::TimePoint start_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));
    test_animation->DoFrame(start_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    // Event test
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_start_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.2f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_end_event());
  }

  // fill mode backwards
  {
    auto test_animation = InitTestAnimation();
    auto css_keyframe_manager = std::make_unique<animation::CSSKeyframeManager>(
        test_animation->GetElement());
    test_animation->BindDelegate(css_keyframe_manager.get());
    test_animation->keyframe_effect()->BindAnimationDelegate(
        css_keyframe_manager.get());
    std::unique_ptr<starlight::AnimationData> animation_data =
        std::make_unique<starlight::AnimationData>();
    animation_data->duration = 1000;
    animation_data->iteration_count = 3;
    animation_data->delay = 1000;
    animation_data->fill_mode = starlight::AnimationFillModeType::kBackwards;
    test_animation->UpdateAnimationData(*animation_data);
    // update start time
    fml::TimePoint start_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(start_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    // Event test
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_start_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.2f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_iteration_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_iteration_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_end_event());
  }
}

TEST_F(AnimationTest, ShouldSendTwoStartEvent) {
  {
    auto test_animation = InitTestAnimation();
    auto css_keyframe_manager = std::make_unique<animation::CSSKeyframeManager>(
        test_animation->GetElement());
    test_animation->BindDelegate(css_keyframe_manager.get());
    test_animation->keyframe_effect()->BindAnimationDelegate(
        css_keyframe_manager.get());
    std::unique_ptr<starlight::AnimationData> animation_data =
        std::make_unique<starlight::AnimationData>();
    animation_data->duration = 1000;
    animation_data->iteration_count = 1;
    animation_data->fill_mode = starlight::AnimationFillModeType::kBoth;
    test_animation->UpdateAnimationData(*animation_data);
    // update start time
    fml::TimePoint start_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(start_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_start_event());

    // Event test
    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.8f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    // update a longer duration, since animation has not end, then it should not
    // have another start event.
    animation_data->duration = 2000;
    animation_data->iteration_count = 1;
    animation_data->fill_mode = starlight::AnimationFillModeType::kForwards;
    test_animation->UpdateAnimationData(*animation_data);

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_end_event());

    // update a longer duration, since animation has end, then it should have
    // another start event.
    animation_data->duration = 3000;
    animation_data->iteration_count = 1;
    animation_data->fill_mode = starlight::AnimationFillModeType::kForwards;
    test_animation->UpdateAnimationData(*animation_data);

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.2f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_start_event());
    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_end_event());

    // update a shorter duration
    animation_data->duration = 2500;
    animation_data->iteration_count = 1;
    animation_data->fill_mode = starlight::AnimationFillModeType::kForwards;
    test_animation->UpdateAnimationData(*animation_data);

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.2f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    // update a zero duration
    animation_data->duration = 0;
    animation_data->iteration_count = 1;
    animation_data->delay = 1000;
    animation_data->fill_mode = starlight::AnimationFillModeType::kForwards;
    test_animation->UpdateAnimationData(*animation_data);

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(4.3f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    // update a zero duration with longer delay
    animation_data->duration = 0;
    animation_data->iteration_count = 1;
    animation_data->delay = 5000;
    animation_data->fill_mode = starlight::AnimationFillModeType::kForwards;
    test_animation->UpdateAnimationData(*animation_data);

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(6.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->has_received_animation_start_event() &&
                tasm_mediator->has_received_animation_end_event());

    // longer delay
    animation_data->duration = 1000;
    animation_data->iteration_count = 1;
    animation_data->delay = 7000;
    animation_data->fill_mode = starlight::AnimationFillModeType::kForwards;
    test_animation->UpdateAnimationData(*animation_data);

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(7.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    // paused at 7.5s, resume at 8.5s, paused time 1s
    test_animation->Pause();
    test_animation->DoFrame(tick_time);
    test_animation->Play();
    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(8.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(9.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->only_received_animation_start_event());

    // cancel at 9.5s
    tasm_mediator->ClearAnimationEvent();
    test_animation->Destroy();
    EXPECT_TRUE(tasm_mediator->only_received_animation_cancel_event());
  }
}

TEST_F(AnimationTest, TestDurationZero) {
  // delay 0
  {
    auto test_animation = InitTestAnimation();
    auto css_keyframe_manager = std::make_unique<animation::CSSKeyframeManager>(
        test_animation->GetElement());
    test_animation->BindDelegate(css_keyframe_manager.get());
    test_animation->keyframe_effect()->BindAnimationDelegate(
        css_keyframe_manager.get());
    std::unique_ptr<starlight::AnimationData> animation_data =
        std::make_unique<starlight::AnimationData>();
    animation_data->duration = 0;
    animation_data->iteration_count = 1;
    animation_data->fill_mode = starlight::AnimationFillModeType::kBackwards;
    test_animation->UpdateAnimationData(*animation_data);
    // update start time
    fml::TimePoint start_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));
    test_animation->DoFrame(start_time);
    EXPECT_TRUE(tasm_mediator->has_received_animation_start_event() &&
                tasm_mediator->has_received_animation_end_event() &&
                !tasm_mediator->has_received_animation_cancel_event());

    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());
  }

  // delay 1000
  {
    auto test_animation = InitTestAnimation();
    auto css_keyframe_manager = std::make_unique<animation::CSSKeyframeManager>(
        test_animation->GetElement());
    test_animation->BindDelegate(css_keyframe_manager.get());
    test_animation->keyframe_effect()->BindAnimationDelegate(
        css_keyframe_manager.get());
    std::unique_ptr<starlight::AnimationData> animation_data =
        std::make_unique<starlight::AnimationData>();
    animation_data->duration = 0;
    animation_data->iteration_count = 1;
    animation_data->delay = 1000;
    animation_data->fill_mode = starlight::AnimationFillModeType::kBackwards;
    test_animation->UpdateAnimationData(*animation_data);
    // update start time
    fml::TimePoint start_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(start_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    fml::TimePoint tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.1f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->has_received_animation_start_event() &&
                tasm_mediator->has_received_animation_end_event() &&
                !tasm_mediator->has_received_animation_cancel_event());

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.0f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    // longer delay 3s
    animation_data->duration = 0;
    animation_data->iteration_count = 1;
    animation_data->delay = 3000;
    animation_data->fill_mode = starlight::AnimationFillModeType::kBackwards;
    test_animation->UpdateAnimationData(*animation_data);

    tick_time =
        fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(3.5f));
    tasm_mediator->ClearAnimationEvent();
    test_animation->Play();
    test_animation->DoFrame(tick_time);
    EXPECT_TRUE(tasm_mediator->not_received_any_event());

    tasm_mediator->ClearAnimationEvent();
    test_animation->Destroy();
    EXPECT_TRUE(tasm_mediator->only_received_animation_cancel_event());
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
