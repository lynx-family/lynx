// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/animation/keyframe_effect.h"

#include <cstdint>
#include <memory>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
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

namespace {

lepus::Value MakeBoxShadowLength(float value) {
  auto array = lepus::CArray::Create();
  array->emplace_back(value);
  array->emplace_back(static_cast<int>(CSSValuePattern::NUMBER));
  return lepus::Value(std::move(array));
}

CSSValue MakeBoxShadowValue(float h_offset, float v_offset, float blur,
                            float spread, uint32_t color) {
  auto group = lepus::CArray::Create();
  auto shadow = lepus::Dictionary::Create();
  shadow->SetValue("enable", true);
  shadow->SetValue("h_offset", MakeBoxShadowLength(h_offset));
  shadow->SetValue("v_offset", MakeBoxShadowLength(v_offset));
  shadow->SetValue("blur", MakeBoxShadowLength(blur));
  shadow->SetValue("spread", MakeBoxShadowLength(spread));
  shadow->SetValue("option", static_cast<int>(starlight::ShadowOption::kNone));
  shadow->SetValue("color", color);
  group->emplace_back(lepus::Value(std::move(shadow)));
  return CSSValue(std::move(group));
}

float GetBoxShadowLength(const CSSValue& value, const char* key) {
  auto shadow = value.GetArray()->get(0).Table();
  auto length = shadow->GetValue(key).Array();
  return length->get(0).Number();
}

uint32_t GetBoxShadowColor(const CSSValue& value) {
  auto shadow = value.GetArray()->get(0).Table();
  return static_cast<uint32_t>(shadow->GetValue("color").Number());
}

}  // namespace

class KeyframeEffectTest : public ::testing::Test {
 public:
  struct GfxEffectBundle {
    std::unique_ptr<lynx::gfx::KeyframeEffect> effect;
    std::vector<std::unique_ptr<lynx::gfx::KeyframeModel>> models;
  };

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
    element_ = manager->CreateFiberElement("view");
    animation_->BindElement(element_.get());
    return keyframe_effect;
  }

  void InitTestEffectInternal(animation::KeyframeEffect* test_effect) {
    // add first model: color model
    std::unique_ptr<animation::KeyframedColorAnimationCurve> test_curve1(
        animation::KeyframedColorAnimationCurve::Create(
            starlight::XAnimationColorInterpolationType::kSRGB));
    auto test_frame1 = gfx::ColorKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetValue(4294901760);
    test_curve1->AddKeyframe(std::move(test_frame1));
    test_curve1->type_ = animation::AnimationCurve::CurveType::OPACITY;
    auto test_frame2 =
        gfx::ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame2->SetValue(4278255360);
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

    auto test_frame5 = gfx::FloatKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame5->SetValue(1.0f);
    test_curve3->AddKeyframe(std::move(test_frame5));
    test_curve3->type_ = animation::AnimationCurve::CurveType::BGCOLOR;
    auto test_frame6 =
        gfx::FloatKeyframe::Create(fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame6->SetValue(0.0f);
    test_curve3->AddKeyframe(std::move(test_frame6));
    std::unique_ptr<animation::KeyframeModel> new_model3 =
        animation::KeyframeModel::Create(std::move(test_curve3));
    test_effect->AddKeyframeModel(std::move(new_model3));
  }

  GfxEffectBundle InitTestGfxEffect() {
    GfxEffectBundle bundle;
    bundle.effect = lynx::gfx::KeyframeEffect::Create();

    std::unique_ptr<animation::KeyframedColorAnimationCurve> test_curve1(
        animation::KeyframedColorAnimationCurve::Create(
            starlight::XAnimationColorInterpolationType::kSRGB));
    auto test_frame1 = gfx::ColorKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetValue(4294901760);
    test_curve1->AddKeyframe(std::move(test_frame1));
    test_curve1->type_ = animation::AnimationCurve::CurveType::OPACITY;
    auto test_frame2 =
        gfx::ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame2->SetValue(4278255360);
    test_curve1->AddKeyframe(std::move(test_frame2));
    bundle.models.push_back(
        lynx::gfx::KeyframeModel::Create(std::move(test_curve1)));
    bundle.effect->AddKeyframeModel(bundle.models.back().get());

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
    bundle.models.push_back(
        lynx::gfx::KeyframeModel::Create(std::move(test_curve2)));
    bundle.effect->AddKeyframeModel(bundle.models.back().get());

    std::unique_ptr<animation::KeyframedOpacityAnimationCurve> test_curve3(
        animation::KeyframedOpacityAnimationCurve::Create());
    auto test_frame5 = gfx::FloatKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame5->SetValue(1.0f);
    test_curve3->AddKeyframe(std::move(test_frame5));
    test_curve3->type_ = animation::AnimationCurve::CurveType::BGCOLOR;
    auto test_frame6 =
        gfx::FloatKeyframe::Create(fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame6->SetValue(0.0f);
    test_curve3->AddKeyframe(std::move(test_frame6));
    bundle.models.push_back(
        lynx::gfx::KeyframeModel::Create(std::move(test_curve3)));
    bundle.effect->AddKeyframeModel(bundle.models.back().get());

    return bundle;
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

  auto test_frame1 = gfx::ColorKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame1->SetValue(4294901760);
  test_curve1->AddKeyframe(std::move(test_frame1));
  test_curve1->type_ = animation::AnimationCurve::CurveType::BGCOLOR;
  auto test_frame2 =
      gfx::ColorKeyframe::Create(fml::TimeDelta::FromSecondsF(4.0), nullptr);
  test_frame2->SetValue(4278255360);
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

  auto test_frame5 = gfx::FloatKeyframe::Create(fml::TimeDelta(), nullptr);
  test_frame5->SetValue(1.0f);
  test_curve3->AddKeyframe(std::move(test_frame5));
  test_curve3->type_ = animation::AnimationCurve::CurveType::OPACITY;
  auto test_frame6 =
      gfx::FloatKeyframe::Create(fml::TimeDelta::FromSecondsF(1.0), nullptr);
  test_frame6->SetValue(0.0f);
  test_curve3->AddKeyframe(std::move(test_frame6));

  std::unique_ptr<animation::KeyframeModel> new_model3 =
      animation::KeyframeModel::Create(std::move(test_curve3));
  test_effect.AddKeyframeModel(std::move(new_model3));

  EXPECT_EQ(3ul, test_effect.keyframe_models().size());
}

TEST_F(KeyframeEffectTest, SetStartTime) {
  auto bundle = InitTestGfxEffect();
  fml::TimePoint test_start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(1.0));
  bundle.effect->SetStartTime(test_start_time);
  for (auto& model : bundle.models) {
    EXPECT_EQ(model->start_time(), fml::TimePoint::FromEpochDelta(
                                       fml::TimeDelta::FromSecondsF(1.0)));
    EXPECT_EQ(model->GetRunState(), gfx::KeyframeModel::STARTING);
  }
}

TEST_F(KeyframeEffectTest, SetPauseTime) {
  auto bundle = InitTestGfxEffect();
  fml::TimePoint test_pause_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0));
  bundle.effect->SetPauseTime(test_pause_time);
  for (auto& model : bundle.models) {
    EXPECT_EQ(model->pause_time(), fml::TimePoint::FromEpochDelta(
                                       fml::TimeDelta::FromSecondsF(2.0)));
    EXPECT_EQ(model->GetRunState(), lynx::gfx::KeyframeModel::PAUSED);
  }
}

TEST_F(KeyframeEffectTest, TickCountsSkippedIterationsAndIgnoresRollback) {
  auto effect = gfx::KeyframeEffect::Create();
  auto curve = animation::KeyframedOpacityAnimationCurve::Create();
  auto from_keyframe = gfx::FloatKeyframe::Create(fml::TimeDelta(), nullptr);
  from_keyframe->SetValue(1.0f);
  curve->AddKeyframe(std::move(from_keyframe));
  auto to_keyframe = gfx::FloatKeyframe::Create(
      fml::TimeDelta::FromMilliseconds(1000), nullptr);
  to_keyframe->SetValue(0.0f);
  curve->AddKeyframe(std::move(to_keyframe));
  auto model = gfx::KeyframeModel::Create(std::move(curve));

  gfx::AnimationData data;
  data.duration = 1000;
  data.delay = 0;
  data.iteration_count = 4;
  model->SetAnimationData(&data);
  effect->AddKeyframeModel(model.get());

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  effect->SetStartTime(start_time);

  fml::TimePoint first_iteration =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(500));
  auto initial_result = effect->Tick(first_iteration);
  EXPECT_EQ(0, initial_result.iteration_events_due);
  EXPECT_EQ(1u, initial_result.samples.size());

  fml::TimePoint third_iteration =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(2500));
  auto skipped_result = effect->Tick(third_iteration);
  EXPECT_EQ(2, skipped_result.iteration_events_due);
  EXPECT_EQ(1u, skipped_result.samples.size());

  fml::TimePoint second_iteration =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(1500));
  auto rollback_result = effect->Tick(second_iteration);
  EXPECT_EQ(0, rollback_result.iteration_events_due);
  EXPECT_EQ(1u, rollback_result.samples.size());
}

TEST_F(KeyframeEffectTest, TickSamplesBoxShadowAnimationCurve) {
  auto effect = gfx::KeyframeEffect::Create();
  auto curve = animation::KeyframedBoxShadowAnimationCurve::Create();
  curve->type_ = animation::AnimationCurve::CurveType::BOX_SHADOW;
  element_ = manager->CreateFiberElement("view");
  curve->SetElement(element_.get());

  auto from_keyframe =
      animation::BoxShadowKeyframe::Create(fml::TimeDelta(), nullptr);
  from_keyframe->SetBoxShadow(
      MakeBoxShadowValue(0.f, 0.f, 0.f, 0.f, 0x80000000));
  curve->AddKeyframe(std::move(from_keyframe));

  auto to_keyframe = animation::BoxShadowKeyframe::Create(
      fml::TimeDelta::FromMilliseconds(1000), nullptr);
  to_keyframe->SetBoxShadow(
      MakeBoxShadowValue(20.f, 10.f, 16.f, 4.f, 0x80000000));
  curve->AddKeyframe(std::move(to_keyframe));

  auto model = gfx::KeyframeModel::Create(std::move(curve));
  gfx::AnimationData data;
  data.duration = 1000;
  data.delay = 0;
  data.iteration_count = 1;
  model->SetAnimationData(&data);
  effect->AddKeyframeModel(model.get());

  fml::TimePoint start_time =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(0));
  effect->SetStartTime(start_time);

  auto tick_result = effect->Tick(
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromMilliseconds(500)));

  ASSERT_TRUE(tick_result.active_time.has_value());
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(500), *tick_result.active_time);
  ASSERT_EQ(1u, tick_result.samples.size());
  EXPECT_EQ(fml::TimeDelta::FromMilliseconds(500),
            tick_result.samples[0].trimmed_time);

  auto* sampled_curve =
      static_cast<animation::AnimationCurve*>(tick_result.samples[0].curve);
  ASSERT_NE(nullptr, sampled_curve);
  EXPECT_EQ(animation::AnimationCurve::CurveType::BOX_SHADOW,
            sampled_curve->Type());

  fml::TimeDelta sample_time = tick_result.samples[0].trimmed_time;
  auto sampled_value = sampled_curve->GetValue(sample_time);
  ASSERT_TRUE(sampled_value.IsArray());
  ASSERT_EQ(1u, sampled_value.GetArray()->size());
  EXPECT_FLOAT_EQ(10.f, GetBoxShadowLength(sampled_value, "h_offset"));
  EXPECT_FLOAT_EQ(5.f, GetBoxShadowLength(sampled_value, "v_offset"));
  EXPECT_FLOAT_EQ(8.f, GetBoxShadowLength(sampled_value, "blur"));
  EXPECT_FLOAT_EQ(2.f, GetBoxShadowLength(sampled_value, "spread"));
  EXPECT_EQ(0x80000000u, GetBoxShadowColor(sampled_value));
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
  test_effect->SetStartTime(start_time, true);

  fml::TimePoint test_time1 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(2.0));
  test_effect->TickKeyframeModel(test_time1);
  bool all_finished = test_effect->HasFinishedAll();
  EXPECT_EQ(false, all_finished);

  fml::TimePoint test_time2 =
      fml::TimePoint::FromEpochDelta(fml::TimeDelta::FromSecondsF(5.0));
  test_effect->TickKeyframeModel(test_time2);
  all_finished = test_effect->HasFinishedAll();
  EXPECT_EQ(true, all_finished);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
