// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/animation/css_keyframe_manager.h"

#include <algorithm>
#include <memory>

#include "core/animation/animation.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/testing/mock_css_keyframe_manager.h"
#include "core/animation/transform_animation_curve.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "gfx/animation/timing_function.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class CSSKeyframeManagerTest : public ::testing::Test {
 public:
  CSSKeyframeManagerTest() {}
  ~CSSKeyframeManagerTest() override {}
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

  std::shared_ptr<animation::Animation> InitTestAnimation() {
    auto test_animation =
        std::make_shared<animation::Animation>("test_animation");
    auto effect = animation::KeyframeEffect::Create();
    test_animation->SetKeyframeEffect(std::move(effect));
    element_ = manager->CreateFiberElement("view");
    test_animation->BindElement(element_.get());
    return test_animation;
  }

  void InitTestEffect(animation::KeyframeEffect& test_effect) {
    std::unique_ptr<animation::KeyframedOpacityAnimationCurve> test_curve(
        animation::KeyframedOpacityAnimationCurve::Create());

    auto test_frame1 =
        animation::OpacityKeyframe::Create(fml::TimeDelta(), nullptr);
    test_frame1->SetOpacity(1.0f);
    test_curve->AddKeyframe(std::move(test_frame1));
    test_curve->type_ = animation::AnimationCurve::CurveType::OPACITY;
    auto test_frame2 = animation::OpacityKeyframe::Create(
        fml::TimeDelta::FromSecondsF(4.0), nullptr);
    test_frame2->SetOpacity(0.0f);
    test_curve->AddKeyframe(std::move(test_frame2));
    std::unique_ptr<animation::KeyframeModel> new_model =
        animation::KeyframeModel::Create(std::move(test_curve));
    test_effect.AddKeyframeModel(std::move(new_model));
  }

  std::unique_ptr<animation::MockCSSKeyframeManager> InitTestKeyframeManager(
      tasm::Element* element) {
    auto test_manager =
        std::make_unique<animation::MockCSSKeyframeManager>(element);
    return test_manager;
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

  fml::RefPtr<Element> InitElement() {
    auto test_element = manager->CreateFiberElement("view");
    test_element->SetAttribute(base::String("enable-new-animator"),
                               lepus::Value("true"));
    return test_element;
  }

  void UpdateOpacityKeyframes(tasm::Element* element, const base::String& name,
                              double from, double to) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("opacity", lepus::Value(from));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("opacity", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateToOnlyOpacityKeyframes(tasm::Element* element,
                                    const base::String& name, double to) {
    auto keyframes = lepus::Dictionary::Create();
    keyframes->SetValue("0", lepus::Value(lepus::Dictionary::Create()));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("opacity", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateLeftKeyframes(tasm::Element* element, const base::String& name,
                           const char* from, const char* to) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("left", lepus::Value(from));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("left", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateToOnlyLeftKeyframes(tasm::Element* element,
                                 const base::String& name, const char* to) {
    auto keyframes = lepus::Dictionary::Create();
    keyframes->SetValue("0", lepus::Value(lepus::Dictionary::Create()));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("left", lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateBorderRadiusKeyframes(tasm::Element* element,
                                   const base::String& name,
                                   const char* property, const char* from,
                                   const char* to) {
    auto keyframes = lepus::Dictionary::Create();
    if (from != nullptr) {
      auto from_frame = lepus::Dictionary::Create();
      from_frame->SetValue(property, lepus::Value(from));
      keyframes->SetValue("0", lepus::Value(from_frame));
    }
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue(property, lepus::Value(to));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateWidthKeyframesWithDirectInheritedVariable(
      tasm::Element* element, const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("width", lepus::Value("var(--base)"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("width", lepus::Value("240px"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateWidthKeyframesWithLocalCustomProperty(tasm::Element* element,
                                                   const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("--x", lepus::Value("var(--base)"));
    from_frame->SetValue("width", lepus::Value("var(--x)"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("--x", lepus::Value("480px"));
    to_frame->SetValue("width", lepus::Value("var(--x)"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  void UpdateCustomPropertyOnlyKeyframes(tasm::Element* element,
                                         const base::String& name) {
    auto keyframes = lepus::Dictionary::Create();
    auto from_frame = lepus::Dictionary::Create();
    from_frame->SetValue("--x", lepus::Value("10px"));
    keyframes->SetValue("0", lepus::Value(from_frame));
    auto to_frame = lepus::Dictionary::Create();
    to_frame->SetValue("--x", lepus::Value("20px"));
    keyframes->SetValue("100", lepus::Value(to_frame));

    lynx::tasm::CSSParserConfigs configs;
    starlight::CSSStyleUtils::UpdateCSSKeyframes(
        *element->keyframes_map_, name, lepus::Value(keyframes), configs);
  }

  animation::LayoutKeyframe* FirstWidthKeyframe(
      animation::MockCSSKeyframeManager* manager, const base::String& name) {
    auto animation_iter = manager->animations_map().find(name);
    if (animation_iter == manager->animations_map().end()) {
      return nullptr;
    }
    auto* model =
        animation_iter->second->keyframe_effect()->GetKeyframeModelByCurveType(
            animation::AnimationCurve::CurveType::WIDTH);
    if (model == nullptr) {
      return nullptr;
    }
    auto* curve = static_cast<animation::KeyframedLayoutAnimationCurve*>(
        model->animation_curve());
    if (curve->keyframes_.empty()) {
      return nullptr;
    }
    return static_cast<animation::LayoutKeyframe*>(curve->keyframes_[0].get());
  }

  const tasm::CSSValue* FindSampledStyle(
      const animation::AnimationSampleForNewPipeline& sample,
      tasm::CSSPropertyID id) {
    auto iter = sample.property_overrides.find(id);
    return iter == sample.property_overrides.end() ? nullptr : &iter->second;
  }

  const tasm::CSSValue* FindSampledCustomProperty(
      const animation::AnimationSampleForNewPipeline& sample,
      const base::String& name) {
    auto iter = sample.custom_property_overrides.find(name);
    return iter == sample.custom_property_overrides.end() ? nullptr
                                                          : &iter->second;
  }

  fml::TimePoint TimePointFromMs(int64_t ms) {
    return fml::TimePoint::FromTicks(ms * 1000 * 1000);
  }
};

TEST_F(CSSKeyframeManagerTest, ConstructModel) {
  auto test_element = manager->CreateFiberElement("view");
  const auto underlying_opacity = CSSValue(0.75f, CSSValuePattern::NUMBER);
  ASSERT_TRUE(test_element->computed_css_style()->SetValue(kPropertyIDOpacity,
                                                           underlying_opacity));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto test_curve = animation::KeyframedOpacityAnimationCurve::Create();
  auto test_type = animation::AnimationCurve::CurveType::OPACITY;
  auto test_animation = InitTestAnimation();
  auto test_model = test_manager->ConstructModel(
      std::move(test_curve), test_type, test_animation.get());
  EXPECT_EQ(test_model->animation_curve()->Type(), test_type);
  EXPECT_EQ(test_model->animation_curve()->timing_function()->GetType(),
            gfx::TimingFunction::Type::LINEAR);
  EXPECT_EQ(test_model->animation_curve()->scaled_duration(),
            test_animation->get_animation_data().duration / 1000.0);
  EXPECT_EQ(test_model->animation_curve()->underlying_value_,
            underlying_opacity);
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineRefreshesUnderlyingValueAfterBaseResetAndUpdate) {
  auto test_element = InitElement();
  auto* base_style = test_element->computed_css_style();
  ASSERT_TRUE(base_style->SetValue(kPropertyIDOpacity,
                                   CSSValue(0.5f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(test_element.get(), base::String("test"), 0);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  auto first_mid_time = TimePointFromMs(1500);
  auto first_mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(first_mid_time);
  const auto* first_mid_opacity =
      FindSampledStyle(first_mid_sample, kPropertyIDOpacity);
  ASSERT_NE(nullptr, first_mid_opacity);
  EXPECT_NEAR(0.25, first_mid_opacity->AsNumber(), 0.001);

  ASSERT_TRUE(base_style->ResetValue(kPropertyIDOpacity));
  EXPECT_EQ(base_style->GetResolvedValues().end(),
            base_style->GetResolvedValues().find(kPropertyIDOpacity));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto reset_mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(first_mid_time);
  const auto* reset_mid_opacity =
      FindSampledStyle(reset_mid_sample, kPropertyIDOpacity);
  ASSERT_NE(nullptr, reset_mid_opacity);
  EXPECT_NEAR(0.5, reset_mid_opacity->AsNumber(), 0.001);

  ASSERT_TRUE(base_style->SetValue(kPropertyIDOpacity,
                                   CSSValue(0.8f, CSSValuePattern::NUMBER)));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto updated_mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(first_mid_time);
  const auto* updated_mid_opacity =
      FindSampledStyle(updated_mid_sample, kPropertyIDOpacity);
  ASSERT_NE(nullptr, updated_mid_opacity);
  EXPECT_NEAR(0.4, updated_mid_opacity->AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest,
       LegacyPipelineRefreshesUnderlyingValueAfterBaseResetAndUpdate) {
  auto test_element = manager->CreateFiberElement("view");
  ASSERT_TRUE(test_element->computed_css_style()->SetValue(
      kPropertyIDOpacity, CSSValue(0.5f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(test_element.get(), base::String("test"), 0);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::OPACITY);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedOpacityAnimationCurve*>(
      model->animation_curve());
  test_element->css_keyframe_manager_ = std::move(test_manager);

  test_element->SetStyleInternal(kPropertyIDOpacity,
                                 CSSValue(0.8f, CSSValuePattern::NUMBER));
  auto mid_time = fml::TimeDelta::FromSecondsF(0.5);
  EXPECT_NEAR(0.4, curve->GetValue(mid_time).AsNumber(), 0.001);

  test_element->FlushAnimatedStyleInternal(
      kPropertyIDOpacity, CSSValue(0.4f, CSSValuePattern::NUMBER));
  EXPECT_NEAR(0.4, curve->GetValue(mid_time).AsNumber(), 0.001);

  test_element->ResetStyleInternal(kPropertyIDOpacity);
  EXPECT_NEAR(0.5, curve->GetValue(mid_time).AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest,
       LegacyLayoutOnlyResetUsesDefaultUnderlyingValue) {
  auto test_element = manager->CreateFiberElement("view");
  ASSERT_FALSE(test_element->EnableLayoutInElementMode());
  UpdateToOnlyLeftKeyframes(test_element.get(), base::String("test"), "100px");

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::LEFT);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedLayoutAnimationCurve*>(
      model->animation_curve());
  test_element->css_keyframe_manager_ = std::move(test_manager);

  test_element->SetStyleInternal(kPropertyIDLeft,
                                 CSSValue(20.f, CSSValuePattern::PX));
  const auto& resolved_values =
      test_element->computed_css_style()->GetResolvedValues();
  EXPECT_EQ(resolved_values.end(), resolved_values.find(kPropertyIDLeft));
  auto mid_time = fml::TimeDelta::FromSecondsF(0.5);
  EXPECT_NEAR(60.f, curve->GetValue(mid_time).AsNumber(), 0.001);

  test_element->ResetStyleInternal(kPropertyIDLeft);
  EXPECT_EQ(resolved_values.end(), resolved_values.find(kPropertyIDLeft));
  EXPECT_NEAR(100.f, curve->GetValue(mid_time).AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest, InitCurveAndModelAndKeyframe) {
  auto test_element = manager->CreateFiberElement("view");
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto test_offset = 0.0;

  auto id1 = lynx::tasm::CSSPropertyID::kPropertyIDLeft;
  lynx::tasm::StyleMap output1;
  lynx::tasm::CSSParserConfigs configs;
  auto impl1 = lepus::Value("100px");
  lynx::tasm::UnitHandler::Process(id1, impl1, output1, configs);
  auto raw_value1 = output1[id1];
  auto test_animation1 = InitTestAnimation();
  auto test_timing_function1 = gfx::LinearTimingFunction::Create();
  auto test_type1 = animation::AnimationCurve::CurveType::LEFT;
  bool init_success1 = test_manager->InitCurveAndModelAndKeyframe(
      test_type1, test_animation1.get(), test_offset,
      std::move(test_timing_function1), id1, raw_value1);
  EXPECT_EQ(init_success1, true);
  auto* model1 = test_animation1->keyframe_effect()->keyframe_models()[0].get();
  ASSERT_NE(nullptr, model1);
  EXPECT_TRUE(model1->HasAnimationData());
  EXPECT_TRUE(model1->get_animation_data() ==
              test_animation1->get_animation_data());

  auto id2 = lynx::tasm::CSSPropertyID::kPropertyIDOpacity;
  lynx::tasm::StyleMap output2;
  auto impl2 = lepus::Value("1");
  lynx::tasm::UnitHandler::Process(id2, impl2, output2, configs);
  auto raw_value2 = output2[id2];
  auto test_animation2 = InitTestAnimation();
  auto test_timing_function2 = gfx::LinearTimingFunction::Create();
  auto test_type2 = animation::AnimationCurve::CurveType::OPACITY;
  bool init_success2 = test_manager->InitCurveAndModelAndKeyframe(
      test_type2, test_animation2.get(), test_offset,
      std::move(test_timing_function2), id2, raw_value2);
  EXPECT_EQ(init_success2, true);
  auto* model2 = test_animation2->keyframe_effect()->keyframe_models()[0].get();
  ASSERT_NE(nullptr, model2);
  EXPECT_TRUE(model2->HasAnimationData());
  EXPECT_TRUE(model2->get_animation_data() ==
              test_animation2->get_animation_data());

  auto id3 = lynx::tasm::CSSPropertyID::kPropertyIDColor;
  lynx::tasm::StyleMap output3;
  auto impl3 = lepus::Value("blue");
  lynx::tasm::UnitHandler::Process(id3, impl3, output3, configs);
  auto raw_value3 = output3[id3];
  auto test_animation3 = InitTestAnimation();
  auto test_timing_function3 = gfx::LinearTimingFunction::Create();
  auto test_type3 = animation::AnimationCurve::CurveType::TEXTCOLOR;
  bool init_success3 = test_manager->InitCurveAndModelAndKeyframe(
      test_type3, test_animation3.get(), test_offset,
      std::move(test_timing_function3), id3, raw_value3);
  EXPECT_EQ(init_success3, true);
  auto* model3 = test_animation3->keyframe_effect()->keyframe_models()[0].get();
  ASSERT_NE(nullptr, model3);
  EXPECT_TRUE(model3->HasAnimationData());
  EXPECT_TRUE(model3->get_animation_data() ==
              test_animation3->get_animation_data());

  auto id4 = lynx::tasm::CSSPropertyID::kPropertyIDColor;
  lynx::tasm::StyleMap output4;
  auto impl4 = lepus::Value("");
  lynx::tasm::UnitHandler::Process(id4, impl4, output4, configs);
  auto raw_value4 = output4[id4];
  auto test_animation4 = InitTestAnimation();
  auto test_timing_function4 = gfx::LinearTimingFunction::Create();
  auto test_type4 = animation::AnimationCurve::CurveType::UNSUPPORT;
  bool init_success4 = test_manager->InitCurveAndModelAndKeyframe(
      test_type4, test_animation4.get(), test_offset,
      std::move(test_timing_function4), id4, raw_value4);
  EXPECT_EQ(init_success4, false);
}

TEST_F(CSSKeyframeManagerTest, AcceptsExpandedBorderRadiusKeyframes) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  const base::String animation_name("radius");

  auto keyframes = lepus::Dictionary::Create();
  auto from_frame = lepus::Dictionary::Create();
  from_frame->SetValue("border-radius", lepus::Value("10px 20px"));
  keyframes->SetValue("0", lepus::Value(from_frame));
  auto to_frame = lepus::Dictionary::Create();
  to_frame->SetValue("border-radius", lepus::Value("30px 40px"));
  keyframes->SetValue("100", lepus::Value(to_frame));

  lynx::tasm::CSSParserConfigs configs;
  starlight::CSSStyleUtils::UpdateCSSKeyframes(
      *test_element->keyframes_map_, animation_name, lepus::Value(keyframes),
      configs);
  auto token_iter = test_element->keyframes_map_->find(animation_name);
  ASSERT_NE(token_iter, test_element->keyframes_map_->end());
  auto& content = token_iter->second->GetKeyframesContent();
  ASSERT_EQ(content.size(), 2U);
  for (const auto& [_, styles] : content) {
    ASSERT_NE(styles, nullptr);
    EXPECT_TRUE(styles->contains(kPropertyIDBorderTopLeftRadius));
    EXPECT_TRUE(styles->contains(kPropertyIDBorderTopRightRadius));
    EXPECT_TRUE(styles->contains(kPropertyIDBorderBottomRightRadius));
    EXPECT_TRUE(styles->contains(kPropertyIDBorderBottomLeftRadius));
  }

  auto data = InitAnimationData(animation_name, 1000, 0,
                                starlight::TimingFunctionData(), 1,
                                starlight::AnimationFillModeType::kNone,
                                starlight::AnimationDirectionType::kNormal,
                                starlight::AnimationPlayStateType::kRunning);
  auto radius_animation = test_manager->CreateAnimation(data);
  ASSERT_NE(radius_animation, nullptr);
  ASSERT_EQ(radius_animation->keyframe_effect()->keyframe_models().size(), 4U);

  EXPECT_NE(radius_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::BORDER_TOP_LEFT_RADIUS),
            nullptr);
  EXPECT_NE(radius_animation->keyframe_effect()->GetKeyframeModelByCurveType(
                animation::AnimationCurve::CurveType::BORDER_TOP_RIGHT_RADIUS),
            nullptr);
  EXPECT_NE(
      radius_animation->keyframe_effect()->GetKeyframeModelByCurveType(
          animation::AnimationCurve::CurveType::BORDER_BOTTOM_RIGHT_RADIUS),
      nullptr);
  EXPECT_NE(
      radius_animation->keyframe_effect()->GetKeyframeModelByCurveType(
          animation::AnimationCurve::CurveType::BORDER_BOTTOM_LEFT_RADIUS),
      nullptr);
}

TEST_F(CSSKeyframeManagerTest, AcceptsOnlyBorderRadiusSingletonKeyframeTable) {
  auto test_element = InitElement();
  lynx::tasm::CSSParserConfigs configs;

  auto radius_keyframes = lepus::Dictionary::Create();
  auto radius_frame = lepus::Dictionary::Create();
  radius_frame->SetValue("border-radius", lepus::Value("10px 20px"));
  radius_keyframes->SetValue("100", lepus::Value(radius_frame));
  const base::String radius_name("radius");
  starlight::CSSStyleUtils::UpdateCSSKeyframes(
      *test_element->keyframes_map_, radius_name,
      lepus::Value(radius_keyframes), configs);
  auto radius_iter = test_element->keyframes_map_->find(radius_name);
  ASSERT_NE(radius_iter, test_element->keyframes_map_->end());
  const auto& radius_content = radius_iter->second->GetKeyframesContent();
  ASSERT_EQ(1U, radius_content.size());
  const auto& radius_styles = radius_content.begin()->second;
  ASSERT_NE(nullptr, radius_styles);
  EXPECT_TRUE(radius_styles->contains(tasm::kPropertyIDBorderTopLeftRadius));
  EXPECT_TRUE(radius_styles->contains(tasm::kPropertyIDBorderTopRightRadius));
  EXPECT_TRUE(
      radius_styles->contains(tasm::kPropertyIDBorderBottomRightRadius));
  EXPECT_TRUE(radius_styles->contains(tasm::kPropertyIDBorderBottomLeftRadius));

  auto opacity_keyframes = lepus::Dictionary::Create();
  auto opacity_frame = lepus::Dictionary::Create();
  opacity_frame->SetValue("opacity", lepus::Value(0.5));
  opacity_keyframes->SetValue("100", lepus::Value(opacity_frame));
  const base::String opacity_name("opacity");
  starlight::CSSStyleUtils::UpdateCSSKeyframes(
      *test_element->keyframes_map_, opacity_name,
      lepus::Value(opacity_keyframes), configs);
  EXPECT_EQ(test_element->keyframes_map_->end(),
            test_element->keyframes_map_->find(opacity_name));
}

TEST_F(CSSKeyframeManagerTest,
       LegacyBorderRadiusImplicitFromUsesCurrentStyleOnInitialFrame) {
  auto test_element = InitElement();
  tasm::StyleMap resolved_styles;
  lynx::tasm::CSSParserConfigs configs;
  tasm::UnitHandler::Process(tasm::kPropertyIDBorderTopLeftRadius,
                             lepus::Value("50%"), resolved_styles, configs);
  test_element->computed_css_style()->SetResolvedValue(
      tasm::kPropertyIDBorderTopLeftRadius,
      resolved_styles.at(tasm::kPropertyIDBorderTopLeftRadius));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  const base::String animation_name("radius");
  UpdateBorderRadiusKeyframes(test_element.get(), animation_name,
                              "border-top-left-radius", nullptr, "100%");

  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      animation_name, 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kForwards,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);

  ASSERT_TRUE(test_element->final_animator_map_.has_value());
  auto iter = test_element->final_animator_map_->find(
      tasm::kPropertyIDBorderTopLeftRadius);
  ASSERT_NE(iter, test_element->final_animator_map_->end());
  auto radius = iter->second.GetArray();
  ASSERT_TRUE(radius);
  EXPECT_FLOAT_EQ(50.f, radius->get(0).Number());
  EXPECT_EQ(static_cast<uint32_t>(tasm::CSSValuePattern::PERCENT),
            static_cast<uint32_t>(radius->get(1).Number()));
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineBorderRadiusImplicitFromUsesCurrentBaseStyle) {
  struct TestCase {
    const char* property;
    tasm::CSSPropertyID id;
  };
  const TestCase test_cases[] = {
      {"border-top-left-radius", tasm::kPropertyIDBorderTopLeftRadius},
      {"border-top-right-radius", tasm::kPropertyIDBorderTopRightRadius},
      {"border-bottom-right-radius", tasm::kPropertyIDBorderBottomRightRadius},
  };
  for (const auto& test_case : test_cases) {
    auto test_element = InitElement();
    test_element->SetStyle(test_case.id, lepus::Value("10px"));
    auto test_manager = InitTestKeyframeManager(test_element.get());
    const base::String animation_name(test_case.property);
    UpdateBorderRadiusKeyframes(test_element.get(), animation_name,
                                test_case.property, nullptr, "100%");

    base::Vector<starlight::AnimationData> animation_data;
    animation_data.emplace_back(InitAnimationData(
        animation_name, 1000, 0, starlight::TimingFunctionData(), 1,
        starlight::AnimationFillModeType::kForwards,
        starlight::AnimationDirectionType::kNormal,
        starlight::AnimationPlayStateType::kRunning));
    tasm::StyleMap new_base_styles;
    lynx::tasm::CSSParserConfigs configs;
    tasm::UnitHandler::Process(test_case.id, lepus::Value("50%"),
                               new_base_styles, configs);

    test_manager->SyncAnimationDataForNewPipeline(animation_data, false,
                                                  &new_base_styles);
    auto start_time = TimePointFromMs(1000);
    auto start_sample =
        test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
    const auto* start_radius = FindSampledStyle(start_sample, test_case.id);
    ASSERT_NE(nullptr, start_radius) << test_case.property;
    auto start_array = start_radius->GetArray();
    ASSERT_TRUE(start_array) << test_case.property;
    EXPECT_FLOAT_EQ(50.f, start_array->get(0).Number()) << test_case.property;
    EXPECT_EQ(static_cast<uint32_t>(tasm::CSSValuePattern::PERCENT),
              static_cast<uint32_t>(start_array->get(1).Number()))
        << test_case.property;
  }
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineBorderRadiusMixedUnitsKeepsTimelineAndForwardsFill) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  const base::String animation_name("radius");
  UpdateBorderRadiusKeyframes(test_element.get(), animation_name,
                              "border-top-left-radius", "10px", "50%");

  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      animation_name, 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kForwards,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto end_time = TimePointFromMs(2000);
  auto end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* end_radius =
      FindSampledStyle(end_sample, tasm::kPropertyIDBorderTopLeftRadius);
  ASSERT_NE(nullptr, end_radius);
  auto end_array = end_radius->GetArray();
  ASSERT_TRUE(end_array);
  EXPECT_FLOAT_EQ(50.f, end_array->get(0).Number());
  EXPECT_EQ(static_cast<uint32_t>(tasm::CSSValuePattern::PERCENT),
            static_cast<uint32_t>(end_array->get(1).Number()));
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);
}

TEST_F(CSSKeyframeManagerTest, GetDefaultValue) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto default_value1 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kLeft);
  EXPECT_EQ(default_value1, tasm::CSSValue());

  auto default_value2 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kOpacity);
  EXPECT_EQ(default_value2,
            tasm::CSSValue(animation::OpacityKeyframe::kDefaultOpacity,
                           tasm::CSSValuePattern::NUMBER));

  auto default_value3 = test_manager->GetDefaultValue(
      starlight::AnimationPropertyType::kBackgroundColor);
  EXPECT_EQ(default_value3,
            tasm::CSSValue(animation::ColorKeyframe::kDefaultBackgroundColor,
                           tasm::CSSValuePattern::NUMBER));

  auto default_value4 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kNone);
  EXPECT_EQ(default_value4, tasm::CSSValue());
}

TEST_F(CSSKeyframeManagerTest, HasTwoSameAnimation) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 3000, 100, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_TRUE(test_manager->animations_map()[base::String("test")]
                  ->get_animation_data()
                  .duration == 3000);
}

TEST_F(CSSKeyframeManagerTest, ClearEffect) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_TRUE(test_manager->animations_map()[base::String("test")]
                  ->get_animation_data()
                  .duration == 2000);
  animation_data.clear();
  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->animations_map().empty());
  EXPECT_TRUE(test_manager->GetClearEffectAnimationName() == "test");
}

TEST_F(CSSKeyframeManagerTest, DurationZero) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 0, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);
  EXPECT_TRUE(test_manager->has_flush_animated_style());
  EXPECT_TRUE(
      test_manager->animations_map()[base::String("test")]->GetState() ==
      animation::Animation::State::kStop);
}

TEST_F(CSSKeyframeManagerTest, DurationLessThanZero) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), -1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);
  ASSERT_EQ(1U, test_manager->animation_data_.size());
  EXPECT_EQ(0, test_manager->animation_data_[0].duration);
  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_EQ(0, test_manager->animations_map()[base::String("test")]
                   ->get_animation_data()
                   .duration);
}

TEST_F(CSSKeyframeManagerTest,
       ResolvesKeyframeCustomPropertyFromElementCustomProperty) {
  auto test_element = InitElement();
  test_element->computed_css_style()->SetCustomProperty(
      base::String("--base"), tasm::CSSValue::MakePlainString("120px"));
  test_element->computed_css_style()->FinalizeCustomProperties();

  auto keyframes = lepus::Dictionary::Create();
  auto from_frame = lepus::Dictionary::Create();
  from_frame->SetValue("--x", lepus::Value("var(--base)"));
  from_frame->SetValue("width", lepus::Value("var(--x)"));
  keyframes->SetValue("0", lepus::Value(from_frame));
  auto to_frame = lepus::Dictionary::Create();
  to_frame->SetValue("--x", lepus::Value("240px"));
  to_frame->SetValue("width", lepus::Value("var(--x)"));
  keyframes->SetValue("100", lepus::Value(to_frame));

  lynx::tasm::CSSParserConfigs configs;
  starlight::CSSStyleUtils::UpdateCSSKeyframes(
      *test_element->keyframes_map_, base::String("test"),
      lepus::Value(keyframes), configs);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::WIDTH);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedLayoutAnimationCurve*>(
      model->animation_curve());
  ASSERT_EQ(2U, curve->keyframes_.size());
  auto* from_keyframe =
      static_cast<animation::LayoutKeyframe*>(curve->keyframes_[0].get());
  EXPECT_EQ(tasm::CSSValuePattern::PX, from_keyframe->CSSValue().GetPattern());
  EXPECT_EQ(120, from_keyframe->CSSValue().AsNumber());
}

TEST_F(CSSKeyframeManagerTest,
       ResolvesDirectInheritedCustomPropertyInKeyframeValue) {
  auto test_element = InitElement();
  test_element->computed_css_style()->SetCustomProperty(
      base::String("--base"), tasm::CSSValue::MakePlainString("120px"));
  test_element->computed_css_style()->FinalizeCustomProperties();
  UpdateWidthKeyframesWithDirectInheritedVariable(test_element.get(),
                                                  base::String("test"));

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  test_manager->SetAnimationDataAndPlay(animation_data);

  auto* from_keyframe =
      FirstWidthKeyframe(test_manager.get(), base::String("test"));
  ASSERT_NE(nullptr, from_keyframe);
  EXPECT_EQ(tasm::CSSValuePattern::PX, from_keyframe->CSSValue().GetPattern());
  EXPECT_EQ(120, from_keyframe->CSSValue().AsNumber());
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineSyncUsesProvidedBaseCustomPropertiesForKeyframes) {
  auto test_element = InitElement();
  test_element->computed_css_style()->SetCustomProperty(
      base::String("--base"), tasm::CSSValue::MakePlainString("120px"));
  test_element->computed_css_style()->FinalizeCustomProperties();
  UpdateWidthKeyframesWithLocalCustomProperty(test_element.get(),
                                              base::String("test"));

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  tasm::CustomPropertiesMap new_base_custom_properties;
  new_base_custom_properties.insert_or_assign(
      base::String("--base"), tasm::CSSValue::MakePlainString("240px"));

  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, true, nullptr, nullptr, &new_base_custom_properties);

  auto* from_keyframe =
      FirstWidthKeyframe(test_manager.get(), base::String("test"));
  ASSERT_NE(nullptr, from_keyframe);
  EXPECT_EQ(tasm::CSSValuePattern::PX, from_keyframe->CSSValue().GetPattern());
  EXPECT_EQ(240, from_keyframe->CSSValue().AsNumber());
}

TEST_F(CSSKeyframeManagerTest,
       CustomPropertyOnlyAnimationDoesNotDuplicateStartEventAfterDummySample) {
  auto test_element = InitElement();
  UpdateCustomPropertyOnlyKeyframes(test_element.get(), base::String("test"));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  auto dummy_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  EXPECT_FALSE(dummy_sample.custom_property_overrides.empty());
  auto dummy_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, dummy_events.size());
  EXPECT_TRUE(dummy_events[0].send_start_event);

  auto real_time = fml::TimePoint::FromTicks(16 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(real_time);
  auto real_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  for (const auto& event : real_events) {
    EXPECT_FALSE(event.send_start_event);
  }
}

TEST_F(CSSKeyframeManagerTest,
       CustomPropertyOnlyAnimationSendsEndOnceAndKeepsRepeatedFillSample) {
  auto test_element = InitElement();
  UpdateCustomPropertyOnlyKeyframes(test_element.get(), base::String("test"));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  auto start_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  const auto* start_value =
      FindSampledCustomProperty(start_sample, base::String("--x"));
  ASSERT_NE(nullptr, start_value);
  EXPECT_TRUE(start_value->AsString().IsEqual("10px"));
  auto start_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, start_events.size());
  EXPECT_TRUE(start_events[0].send_start_event);

  auto end_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  auto end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* end_value =
      FindSampledCustomProperty(end_sample, base::String("--x"));
  ASSERT_NE(nullptr, end_value);
  EXPECT_TRUE(end_value->AsString().IsEqual("20px"));
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);

  auto repeated_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* repeated_value =
      FindSampledCustomProperty(repeated_sample, base::String("--x"));
  ASSERT_NE(nullptr, repeated_value);
  EXPECT_TRUE(repeated_value->AsString().IsEqual("20px"));
  auto repeated_events =
      test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(repeated_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       RemovingCustomPropertyOnlyAnimationQueuesResetAndCancelForNewPipeline) {
  auto test_element = InitElement();
  UpdateCustomPropertyOnlyKeyframes(test_element.get(), base::String("test"));
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  base::Vector<starlight::AnimationData> empty_animation_data;
  test_manager->SyncAnimationDataForNewPipeline(empty_animation_data);
  EXPECT_TRUE(test_manager->animations_map().empty());

  auto cleanup_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  EXPECT_TRUE(cleanup_sample.requires_base_style_rebuild);
  EXPECT_NE(std::find(cleanup_sample.custom_property_resets.begin(),
                      cleanup_sample.custom_property_resets.end(),
                      base::String("--x")),
            cleanup_sample.custom_property_resets.end());
  auto cancel_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, cancel_events.size());
  EXPECT_TRUE(cancel_events[0].send_cancel_event);
}

TEST_F(CSSKeyframeManagerTest,
       PausedAnimationKeepsLastSampleWhenNextResolveUsesDummyTime) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto mid_time = fml::TimePoint::FromTicks(1500 * 1000 * 1000);
  auto mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* mid_opacity =
      FindSampledStyle(mid_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, mid_opacity);
  EXPECT_NEAR(0.5, mid_opacity->AsNumber(), 0.001);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  animation_data[0].play_state = starlight::AnimationPlayStateType::kPaused;
  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  auto paused_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  const auto* paused_opacity =
      FindSampledStyle(paused_sample, tasm::kPropertyIDOpacity);

  ASSERT_NE(nullptr, paused_opacity);
  EXPECT_NEAR(mid_opacity->AsNumber(), paused_opacity->AsNumber(), 0.001);
}

TEST_F(CSSKeyframeManagerTest,
       PausedAnimationRefreshesUnderlyingValueAtFrozenSampleTime) {
  auto test_element = InitElement();
  auto* base_style = test_element->computed_css_style();
  ASSERT_TRUE(base_style->SetValue(kPropertyIDOpacity,
                                   CSSValue(0.5f, CSSValuePattern::NUMBER)));
  UpdateToOnlyOpacityKeyframes(test_element.get(), base::String("test"), 0);

  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), -1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  auto mid_time = TimePointFromMs(1500);
  auto mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* mid_opacity =
      FindSampledStyle(mid_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, mid_opacity);
  EXPECT_NEAR(0.25, mid_opacity->AsNumber(), 0.001);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  animation_data[0].play_state = starlight::AnimationPlayStateType::kPaused;
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  ASSERT_TRUE(base_style->ResetValue(kPropertyIDOpacity));
  test_manager->SyncAnimationDataForNewPipeline(
      animation_data, false, &base_style->GetResolvedValues(), nullptr, nullptr,
      base_style);
  auto refreshed_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  const auto* refreshed_opacity =
      FindSampledStyle(refreshed_sample, tasm::kPropertyIDOpacity);

  ASSERT_NE(nullptr, refreshed_opacity);
  EXPECT_NEAR(0.5, refreshed_opacity->AsNumber(), 0.001);
  EXPECT_TRUE(test_manager->TakePendingAnimationEventsForNewPipeline().empty());
}

TEST_F(CSSKeyframeManagerTest,
       PausedAnimationWithoutHistoryIgnoresDummyTimeResolve) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kPaused));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto animation = test_manager->animations_map()[base::String("test")];
  ASSERT_EQ(animation::Animation::State::kPause, animation->GetState());

  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  auto dummy_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);

  EXPECT_TRUE(dummy_sample.empty());
  EXPECT_EQ(animation->pause_time(), fml::TimePoint::Min());
  EXPECT_TRUE(test_manager->TakePendingAnimationEventsForNewPipeline().empty());
}

TEST_F(CSSKeyframeManagerTest,
       StoppedAnimationDoesNotEmitEventsAfterDummyResolve) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto end_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);

  auto dummy_time = animation::Animation::GetAnimationDummyStartTime();
  test_manager->CollectAnimationUpdatesForNewPipeline(dummy_time);
  auto dummy_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(dummy_events.empty());

  auto later_time = fml::TimePoint::FromTicks(2016 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(later_time);
  auto later_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(later_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineReplayFromStoppedAnimationClearsPauseTiming) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 4000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto animation = test_manager->animations_map()[base::String("test")];

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  animation->Pause();
  auto pause_time = fml::TimePoint::FromTicks(1500 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(pause_time);
  EXPECT_EQ(animation->pause_time(), pause_time);

  animation->Stop();
  auto updated_animation_data = animation_data;
  updated_animation_data[0].duration = 5000;
  test_manager->SyncAnimationDataForNewPipeline(updated_animation_data);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  EXPECT_EQ(animation.get(),
            test_manager->animations_map()[base::String("test")].get());
  EXPECT_EQ(animation->pause_time(), fml::TimePoint::Min());
  EXPECT_EQ(animation->total_paused_duration(), fml::TimeDelta::Zero());

  auto next_frame_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(next_frame_time);
  EXPECT_EQ(animation->total_paused_duration(), fml::TimeDelta::Zero());
}

TEST_F(CSSKeyframeManagerTest,
       FinishedAnimationKeepsSampledStyleForSameTimestampResolve) {
  auto test_element = InitElement();
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.8);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = fml::TimePoint::FromTicks(1000 * 1000 * 1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto end_time = fml::TimePoint::FromTicks(2000 * 1000 * 1000);
  auto end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* end_opacity =
      FindSampledStyle(end_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, end_opacity);
  EXPECT_NEAR(0.8, end_opacity->AsNumber(), 0.001);
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);

  auto repeated_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* repeated_opacity =
      FindSampledStyle(repeated_sample, tasm::kPropertyIDOpacity);
  ASSERT_NE(nullptr, repeated_opacity);
  EXPECT_NEAR(end_opacity->AsNumber(), repeated_opacity->AsNumber(), 0.001);
  auto repeated_events =
      test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(repeated_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       ForwardsFillPersistsAfterStoppedAnimationLaterResolve) {
  auto test_element = InitElement();
  UpdateLeftKeyframes(test_element.get(), base::String("move"), "0px", "200px");
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("move"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kForwards,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto end_time = TimePointFromMs(2000);
  auto end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(end_time);
  const auto* end_left = FindSampledStyle(end_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, end_left);
  EXPECT_EQ(*end_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto end_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, end_events.size());
  EXPECT_TRUE(end_events[0].send_end_event);
  ASSERT_TRUE(test_manager->animations_map().count(base::String("move")));
  EXPECT_EQ(animation::Animation::State::kStop,
            test_manager->animations_map()[base::String("move")]->GetState());

  auto later_time = TimePointFromMs(2016);
  auto later_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(later_time);
  const auto* later_left =
      FindSampledStyle(later_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, later_left);
  EXPECT_EQ(*later_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto later_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(later_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       CancelledRestartedForwardsAnimationPersistsSecondFinish) {
  auto test_element = InitElement();
  test_element->SetStyle(tasm::kPropertyIDLeft, lepus::Value("0px"));
  UpdateLeftKeyframes(test_element.get(), base::String("move"), "0px", "200px");
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("move"), 1000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kForwards,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));
  test_manager->SyncAnimationDataForNewPipeline(animation_data);

  auto start_time = TimePointFromMs(1000);
  test_manager->CollectAnimationUpdatesForNewPipeline(start_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto mid_time = TimePointFromMs(1500);
  auto mid_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* mid_left = FindSampledStyle(mid_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, mid_left);

  base::Vector<starlight::AnimationData> empty_animation_data;
  test_manager->SyncAnimationDataForNewPipeline(empty_animation_data);
  auto cleanup_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(mid_time);
  const auto* cleanup_left =
      FindSampledStyle(cleanup_sample, tasm::kPropertyIDLeft);
  auto cleanup_reset_iter =
      std::find(cleanup_sample.property_resets.begin(),
                cleanup_sample.property_resets.end(), tasm::kPropertyIDLeft);
  EXPECT_TRUE(cleanup_left != nullptr ||
              cleanup_reset_iter != cleanup_sample.property_resets.end());
  if (cleanup_left != nullptr) {
    EXPECT_EQ(*cleanup_left, tasm::CSSValue(0, tasm::CSSValuePattern::PX));
    EXPECT_EQ(cleanup_reset_iter, cleanup_sample.property_resets.end());
  }
  auto cancel_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, cancel_events.size());
  EXPECT_TRUE(cancel_events[0].send_cancel_event);

  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  auto restart_time = TimePointFromMs(3000);
  test_manager->CollectAnimationUpdatesForNewPipeline(restart_time);
  test_manager->TakePendingAnimationEventsForNewPipeline();

  auto second_end_time = TimePointFromMs(4000);
  auto second_end_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(second_end_time);
  const auto* second_end_left =
      FindSampledStyle(second_end_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, second_end_left);
  EXPECT_EQ(*second_end_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto second_end_events =
      test_manager->TakePendingAnimationEventsForNewPipeline();
  ASSERT_EQ(1U, second_end_events.size());
  EXPECT_TRUE(second_end_events[0].send_end_event);

  auto later_time = TimePointFromMs(4016);
  auto later_sample =
      test_manager->CollectAnimationUpdatesForNewPipeline(later_time);
  const auto* later_left =
      FindSampledStyle(later_sample, tasm::kPropertyIDLeft);
  ASSERT_NE(nullptr, later_left);
  EXPECT_EQ(*later_left, tasm::CSSValue(200, tasm::CSSValuePattern::PX));
  auto later_events = test_manager->TakePendingAnimationEventsForNewPipeline();
  EXPECT_TRUE(later_events.empty());
}

TEST_F(CSSKeyframeManagerTest,
       NewPipelineForceRebuildRecreatesUnchangedAnimationCurves) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));

  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.8, 0.9);
  test_manager->SyncAnimationDataForNewPipeline(animation_data, true);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::OPACITY);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedOpacityAnimationCurve*>(
      model->animation_curve());
  ASSERT_EQ(2U, curve->keyframes_.size());
  auto* from_keyframe =
      static_cast<animation::OpacityKeyframe*>(curve->keyframes_[0].get());
  EXPECT_FLOAT_EQ(0.8f, from_keyframe->Value());
}

TEST_F(
    CSSKeyframeManagerTest,
    NewPipelineForceRebuildRecreatesAnimationCurvesWhenAnimationDataChanges) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  base::Vector<starlight::AnimationData> animation_data;
  animation_data.emplace_back(InitAnimationData(
      base::String("test"), 2000, 0, starlight::TimingFunctionData(), 1,
      starlight::AnimationFillModeType::kBoth,
      starlight::AnimationDirectionType::kNormal,
      starlight::AnimationPlayStateType::kRunning));

  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.2, 0.4);
  test_manager->SyncAnimationDataForNewPipeline(animation_data);
  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));

  auto updated_animation_data = animation_data;
  updated_animation_data[0].duration = 3000;
  UpdateOpacityKeyframes(test_element.get(), base::String("test"), 0.8, 0.9);
  test_manager->SyncAnimationDataForNewPipeline(updated_animation_data, true);

  ASSERT_TRUE(test_manager->animations_map().count(base::String("test")));
  auto* model = test_manager->animations_map()[base::String("test")]
                    ->keyframe_effect()
                    ->GetKeyframeModelByCurveType(
                        animation::AnimationCurve::CurveType::OPACITY);
  ASSERT_NE(nullptr, model);
  auto* curve = static_cast<animation::KeyframedOpacityAnimationCurve*>(
      model->animation_curve());
  ASSERT_EQ(2U, curve->keyframes_.size());
  auto* from_keyframe =
      static_cast<animation::OpacityKeyframe*>(curve->keyframes_[0].get());
  EXPECT_FLOAT_EQ(0.8f, from_keyframe->Value());
}

TEST_F(CSSKeyframeManagerTest, UpdateAndFlushAnimatedStyle) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());

  auto id = lynx::tasm::CSSPropertyID::kPropertyIDLeft;
  lynx::tasm::StyleMap test_map;
  lynx::tasm::CSSParserConfigs configs;
  auto impl = lepus::Value("100px");
  lynx::tasm::UnitHandler::Process(id, impl, test_map, configs);
  const auto& final_map = *test_element->final_animator_map_;

  bool update_flag = final_map.empty();
  EXPECT_TRUE(update_flag);

  auto [flush_flag, has_pending] = test_element->FlushAnimatedStyle();
  EXPECT_FALSE(flush_flag);

  test_manager->UpdateFinalStyleMap(test_map);
  update_flag = test_element->final_animator_map_->empty();
  EXPECT_FALSE(update_flag);

  std::tie(flush_flag, has_pending) = test_element->FlushAnimatedStyle();
  EXPECT_TRUE(flush_flag);
}

TEST_F(CSSKeyframeManagerTest, SetNeedsAnimationStyleRecalc) {
  auto test_animation = InitTestAnimation();
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  test_manager->SetNeedsAnimationStyleRecalc(test_animation->name());
  const auto& final_map = *test_element->final_animator_map_;
  bool update_flag = final_map.empty();
  EXPECT_TRUE(update_flag);
}

TEST_F(CSSKeyframeManagerTest, GetLayoutPropertyTypeSet) {
  auto test_set = animation::GetLayoutPropertyTypeSet();
  static const base::NoDestructor<
      std::unordered_set<starlight::AnimationPropertyType>>
      base_set({starlight::AnimationPropertyType::kWidth,
                starlight::AnimationPropertyType::kHeight,
                starlight::AnimationPropertyType::kTop,
                starlight::AnimationPropertyType::kLeft,
                starlight::AnimationPropertyType::kRight,
                starlight::AnimationPropertyType::kBottom,
                starlight::AnimationPropertyType::kBorderLeftWidth,
                starlight::AnimationPropertyType::kBorderRightWidth,
                starlight::AnimationPropertyType::kBorderTopWidth,
                starlight::AnimationPropertyType::kBorderBottomWidth,
                starlight::AnimationPropertyType::kPaddingLeft,
                starlight::AnimationPropertyType::kPaddingRight,
                starlight::AnimationPropertyType::kPaddingTop,
                starlight::AnimationPropertyType::kPaddingBottom,
                starlight::AnimationPropertyType::kMarginLeft,
                starlight::AnimationPropertyType::kMarginRight,
                starlight::AnimationPropertyType::kMarginTop,
                starlight::AnimationPropertyType::kMarginBottom,
                starlight::AnimationPropertyType::kMaxWidth,
                starlight::AnimationPropertyType::kMinWidth,
                starlight::AnimationPropertyType::kMaxHeight,
                starlight::AnimationPropertyType::kMinHeight,
                starlight::AnimationPropertyType::kFlexBasis});
  EXPECT_EQ(test_set, *base_set);
}

TEST_F(CSSKeyframeManagerTest, GetLayoutCurveTypeSet) {
  auto test_set = animation::GetLayoutCurveTypeSet();
  static const base::NoDestructor<
      std::unordered_set<animation::AnimationCurve::CurveType>>
      base_set({animation::AnimationCurve::CurveType::LEFT,
                animation::AnimationCurve::CurveType::RIGHT,
                animation::AnimationCurve::CurveType::TOP,
                animation::AnimationCurve::CurveType::BOTTOM,
                animation::AnimationCurve::CurveType::HEIGHT,
                animation::AnimationCurve::CurveType::WIDTH,
                animation::AnimationCurve::CurveType::MAX_WIDTH,
                animation::AnimationCurve::CurveType::MIN_WIDTH,
                animation::AnimationCurve::CurveType::MAX_HEIGHT,
                animation::AnimationCurve::CurveType::MIN_HEIGHT,
                animation::AnimationCurve::CurveType::PADDING_LEFT,
                animation::AnimationCurve::CurveType::PADDING_RIGHT,
                animation::AnimationCurve::CurveType::PADDING_TOP,
                animation::AnimationCurve::CurveType::PADDING_BOTTOM,
                animation::AnimationCurve::CurveType::MARGIN_LEFT,
                animation::AnimationCurve::CurveType::MARGIN_RIGHT,
                animation::AnimationCurve::CurveType::MARGIN_TOP,
                animation::AnimationCurve::CurveType::MARGIN_BOTTOM,
                animation::AnimationCurve::CurveType::BORDER_LEFT_WIDTH,
                animation::AnimationCurve::CurveType::BORDER_RIGHT_WIDTH,
                animation::AnimationCurve::CurveType::BORDER_TOP_WIDTH,
                animation::AnimationCurve::CurveType::BORDER_BOTTOM_WIDTH,
                animation::AnimationCurve::CurveType::FLEX_BASIS});
  EXPECT_EQ(test_set, *base_set);
}

TEST_F(CSSKeyframeManagerTest, GetPropertyIDToAnimationPropertyTypeMap) {
  auto test_map = animation::GetPropertyIDToAnimationPropertyTypeMap();
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      base_map({
          {tasm::kPropertyIDLeft, starlight::AnimationPropertyType::kLeft},
          {tasm::kPropertyIDTop, starlight::AnimationPropertyType::kTop},
          {tasm::kPropertyIDRight, starlight::AnimationPropertyType::kRight},
          {tasm::kPropertyIDBottom, starlight::AnimationPropertyType::kBottom},
          {tasm::kPropertyIDWidth, starlight::AnimationPropertyType::kWidth},
          {tasm::kPropertyIDHeight, starlight::AnimationPropertyType::kHeight},
          {tasm::kPropertyIDOpacity,
           starlight::AnimationPropertyType::kOpacity},
          {tasm::kPropertyIDBackgroundColor,
           starlight::AnimationPropertyType::kBackgroundColor},
          {tasm::kPropertyIDColor, starlight::AnimationPropertyType::kColor},
          {tasm::kPropertyIDMaxWidth,
           starlight::AnimationPropertyType::kMaxWidth},
          {tasm::kPropertyIDMinWidth,
           starlight::AnimationPropertyType::kMinWidth},
          {tasm::kPropertyIDMaxHeight,
           starlight::AnimationPropertyType::kMaxHeight},
          {tasm::kPropertyIDMinHeight,
           starlight::AnimationPropertyType::kMinHeight},
          {tasm::kPropertyIDMarginLeft,
           starlight::AnimationPropertyType::kMarginLeft},
          {tasm::kPropertyIDMarginRight,
           starlight::AnimationPropertyType::kMarginRight},
          {tasm::kPropertyIDMarginTop,
           starlight::AnimationPropertyType::kMarginTop},
          {tasm::kPropertyIDMarginBottom,
           starlight::AnimationPropertyType::kMarginBottom},
          {tasm::kPropertyIDPaddingLeft,
           starlight::AnimationPropertyType::kPaddingLeft},
          {tasm::kPropertyIDPaddingRight,
           starlight::AnimationPropertyType::kPaddingRight},
          {tasm::kPropertyIDPaddingTop,
           starlight::AnimationPropertyType::kPaddingTop},
          {tasm::kPropertyIDPaddingBottom,
           starlight::AnimationPropertyType::kPaddingBottom},
          {tasm::kPropertyIDBorderLeftWidth,
           starlight::AnimationPropertyType::kBorderLeftWidth},
          {tasm::kPropertyIDBorderRightWidth,
           starlight::AnimationPropertyType::kBorderRightWidth},
          {tasm::kPropertyIDBorderTopWidth,
           starlight::AnimationPropertyType::kBorderTopWidth},
          {tasm::kPropertyIDBorderBottomWidth,
           starlight::AnimationPropertyType::kBorderBottomWidth},
          {tasm::kPropertyIDBorderLeftColor,
           starlight::AnimationPropertyType::kBorderLeftColor},
          {tasm::kPropertyIDBorderRightColor,
           starlight::AnimationPropertyType::kBorderRightColor},
          {tasm::kPropertyIDBorderTopColor,
           starlight::AnimationPropertyType::kBorderTopColor},
          {tasm::kPropertyIDBorderBottomColor,
           starlight::AnimationPropertyType::kBorderBottomColor},
          {tasm::kPropertyIDFlexGrow,
           starlight::AnimationPropertyType::kFlexGrow},
          {tasm::kPropertyIDFlexBasis,
           starlight::AnimationPropertyType::kFlexBasis},
          {tasm::kPropertyIDFilter, starlight::AnimationPropertyType::kFilter},
          {tasm::kPropertyIDBoxShadow,
           starlight::AnimationPropertyType::kBoxShadow},
          {tasm::kPropertyIDOffsetDistance,
           starlight::AnimationPropertyType::kOffsetDistance},
          {tasm::kPropertyIDTransform,
           starlight::AnimationPropertyType::kTransform},
          {tasm::kPropertyIDBackgroundPosition,
           starlight::AnimationPropertyType::kBackgroundPosition},
          {tasm::kPropertyIDTransformOrigin,
           starlight::AnimationPropertyType::kTransformOrigin},
          {tasm::kPropertyIDVisibility,
           starlight::AnimationPropertyType::kVisibility},
          {tasm::kPropertyIDBorderTopLeftRadius,
           starlight::AnimationPropertyType::kBorderTopLeftRadius},
          {tasm::kPropertyIDBorderTopRightRadius,
           starlight::AnimationPropertyType::kBorderTopRightRadius},
          {tasm::kPropertyIDBorderBottomRightRadius,
           starlight::AnimationPropertyType::kBorderBottomRightRadius},
          {tasm::kPropertyIDBorderBottomLeftRadius,
           starlight::AnimationPropertyType::kBorderBottomLeftRadius},
      });
  EXPECT_EQ(test_map, *base_map);
}

TEST_F(CSSKeyframeManagerTest, GetAnimatablePropertyIDSet) {
  auto test_set = animation::GetAnimatablePropertyIDSet();
  static const base::NoDestructor<std::unordered_set<tasm::CSSPropertyID>>
      base_set({
          tasm::kPropertyIDTop,
          tasm::kPropertyIDLeft,
          tasm::kPropertyIDRight,
          tasm::kPropertyIDBottom,
          tasm::kPropertyIDWidth,
          tasm::kPropertyIDHeight,
          tasm::kPropertyIDBackgroundColor,
          tasm::kPropertyIDColor,
          tasm::kPropertyIDOpacity,
          tasm::kPropertyIDBorderLeftColor,
          tasm::kPropertyIDBorderRightColor,
          tasm::kPropertyIDBorderTopColor,
          tasm::kPropertyIDBorderBottomColor,
          tasm::kPropertyIDBorderLeftWidth,
          tasm::kPropertyIDBorderRightWidth,
          tasm::kPropertyIDBorderTopWidth,
          tasm::kPropertyIDBorderBottomWidth,
          tasm::kPropertyIDPaddingLeft,
          tasm::kPropertyIDPaddingRight,
          tasm::kPropertyIDPaddingTop,
          tasm::kPropertyIDPaddingBottom,
          tasm::kPropertyIDMarginLeft,
          tasm::kPropertyIDMarginRight,
          tasm::kPropertyIDMarginTop,
          tasm::kPropertyIDMarginBottom,
          tasm::kPropertyIDMaxWidth,
          tasm::kPropertyIDMinWidth,
          tasm::kPropertyIDMaxHeight,
          tasm::kPropertyIDMinHeight,
          tasm::kPropertyIDFlexGrow,
          tasm::kPropertyIDFlexBasis,
          tasm::kPropertyIDTransform,
          tasm::kPropertyIDFilter,
          tasm::kPropertyIDBoxShadow,
          tasm::kPropertyIDOffsetDistance,
          tasm::kPropertyIDBackgroundPosition,
          tasm::kPropertyIDTransformOrigin,
          tasm::kPropertyIDVisibility,
          tasm::kPropertyIDBorderTopLeftRadius,
          tasm::kPropertyIDBorderTopRightRadius,
          tasm::kPropertyIDBorderBottomRightRadius,
          tasm::kPropertyIDBorderBottomLeftRadius,
      });
  EXPECT_EQ(test_set, *base_set);
  bool test_flag = animation::IsAnimatableProperty(tasm::kPropertyIDOpacity);
  bool base_flag = *base_set->find(tasm::kPropertyIDOpacity);
  EXPECT_EQ(test_flag, base_flag);
}

TEST_F(CSSKeyframeManagerTest,
       GetPolymericPropertyIDToAnimationPropertyTypeMap) {
  auto test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kBorderWidth);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyBorderWidthMap({
          {tasm::kPropertyIDBorderTopWidth,
           starlight::AnimationPropertyType::kBorderTopWidth},
          {tasm::kPropertyIDBorderLeftWidth,
           starlight::AnimationPropertyType::kBorderLeftWidth},
          {tasm::kPropertyIDBorderRightWidth,
           starlight::AnimationPropertyType::kBorderRightWidth},
          {tasm::kPropertyIDBorderBottomWidth,
           starlight::AnimationPropertyType::kBorderBottomWidth},
      });
  EXPECT_EQ(test_map, *kIDPropertyBorderWidthMap);

  test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kBorderColor);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyBorderColorMap({
          {tasm::kPropertyIDBorderTopColor,
           starlight::AnimationPropertyType::kBorderTopColor},
          {tasm::kPropertyIDBorderLeftColor,
           starlight::AnimationPropertyType::kBorderLeftColor},
          {tasm::kPropertyIDBorderRightColor,
           starlight::AnimationPropertyType::kBorderRightColor},
          {tasm::kPropertyIDBorderBottomColor,
           starlight::AnimationPropertyType::kBorderBottomColor},
      });
  EXPECT_EQ(test_map, *kIDPropertyBorderColorMap);

  test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kMargin);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyMarginMap({
          {tasm::kPropertyIDMarginTop,
           starlight::AnimationPropertyType::kMarginTop},
          {tasm::kPropertyIDMarginLeft,
           starlight::AnimationPropertyType::kMarginLeft},
          {tasm::kPropertyIDMarginRight,
           starlight::AnimationPropertyType::kMarginRight},
          {tasm::kPropertyIDMarginBottom,
           starlight::AnimationPropertyType::kMarginBottom},
      });
  EXPECT_EQ(test_map, *kIDPropertyMarginMap);

  test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kPadding);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyPaddingMap({
          {tasm::kPropertyIDPaddingTop,
           starlight::AnimationPropertyType::kPaddingTop},
          {tasm::kPropertyIDPaddingLeft,
           starlight::AnimationPropertyType::kPaddingLeft},
          {tasm::kPropertyIDPaddingRight,
           starlight::AnimationPropertyType::kPaddingRight},
          {tasm::kPropertyIDPaddingBottom,
           starlight::AnimationPropertyType::kPaddingBottom},
      });
  EXPECT_EQ(test_map, *kIDPropertyPaddingMap);

  test_map = animation::GetPolymericPropertyIDToAnimationPropertyTypeMap(
      starlight::AnimationPropertyType::kBorderRadius);
  static const base::NoDestructor<
      std::unordered_map<tasm::CSSPropertyID, starlight::AnimationPropertyType>>
      kIDPropertyBorderRadiusMap({
          {tasm::kPropertyIDBorderTopLeftRadius,
           starlight::AnimationPropertyType::kBorderTopLeftRadius},
          {tasm::kPropertyIDBorderTopRightRadius,
           starlight::AnimationPropertyType::kBorderTopRightRadius},
          {tasm::kPropertyIDBorderBottomRightRadius,
           starlight::AnimationPropertyType::kBorderBottomRightRadius},
          {tasm::kPropertyIDBorderBottomLeftRadius,
           starlight::AnimationPropertyType::kBorderBottomLeftRadius},
      });
  EXPECT_EQ(test_map, *kIDPropertyBorderRadiusMap);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
