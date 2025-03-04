// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/animation/css_keyframe_manager.h"

#include <memory>

#include "core/animation/animation.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/testing/mock_css_keyframe_manager.h"
#include "core/animation/transform_animation_curve.h"
#include "core/animation/utils/timing_function.h"
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
    element_ = manager->CreateNode("view", nullptr);
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
    auto test_element = manager->CreateNode("view", nullptr);
    test_element->SetAttribute(base::String("enable-new-animator"),
                               lepus::Value("true"));
    return test_element;
  }
};

TEST_F(CSSKeyframeManagerTest, ConstructModel) {
  auto test_element = manager->CreateNode("view", nullptr);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto test_curve = animation::KeyframedOpacityAnimationCurve::Create();
  auto test_type = animation::AnimationCurve::CurveType::OPACITY;
  auto test_animation = InitTestAnimation();
  auto test_model = test_manager->ConstructModel(
      std::move(test_curve), test_type, test_animation.get());
  EXPECT_EQ(test_model->animation_curve()->Type(), test_type);
  EXPECT_EQ(test_model->animation_curve()->timing_function()->GetType(),
            animation::CubicBezierTimingFunction::MakeTimingFunction(
                test_animation->animation_data()->timing_func)
                .get()
                ->GetType());
  EXPECT_EQ(test_model->animation_curve()->scaled_duration(),
            test_animation->animation_data()->duration / 1000.0);
}

TEST_F(CSSKeyframeManagerTest, InitCurveAndModelAndKeyframe) {
  auto test_element = manager->CreateNode("view", nullptr);
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto test_offset = 0.0;

  auto id1 = lynx::tasm::CSSPropertyID::kPropertyIDLeft;
  lynx::tasm::StyleMap output1;
  lynx::tasm::CSSParserConfigs configs;
  auto impl1 = lepus::Value("100px");
  lynx::tasm::UnitHandler::Process(id1, impl1, output1, configs);
  auto raw_value1 = output1[id1];
  auto test_value_pair1 = std::make_pair(id1, raw_value1);
  auto test_animation1 = InitTestAnimation();
  auto test_timing_function1 =
      animation::CubicBezierTimingFunction::MakeTimingFunction(
          test_animation1->animation_data()->timing_func);
  auto test_type1 = animation::AnimationCurve::CurveType::LEFT;
  bool init_success1 = test_manager->InitCurveAndModelAndKeyframe(
      test_type1, test_animation1.get(), test_offset,
      std::move(test_timing_function1), test_value_pair1);
  EXPECT_EQ(init_success1, true);
  EXPECT_EQ(*test_animation1->animation_data(),
            test_animation1->keyframe_effect()
                ->keyframe_models()[0]
                ->get_animation_data());

  auto id2 = lynx::tasm::CSSPropertyID::kPropertyIDOpacity;
  lynx::tasm::StyleMap output2;
  auto impl2 = lepus::Value("1");
  lynx::tasm::UnitHandler::Process(id2, impl2, output2, configs);
  auto raw_value2 = output2[id2];
  auto test_value_pair2 = std::make_pair(id2, raw_value2);
  auto test_animation2 = InitTestAnimation();
  auto test_timing_function2 =
      animation::CubicBezierTimingFunction::MakeTimingFunction(
          test_animation2->animation_data()->timing_func);
  auto test_type2 = animation::AnimationCurve::CurveType::OPACITY;
  bool init_success2 = test_manager->InitCurveAndModelAndKeyframe(
      test_type2, test_animation2.get(), test_offset,
      std::move(test_timing_function2), test_value_pair2);
  EXPECT_EQ(init_success2, true);
  EXPECT_EQ(*test_animation2->animation_data(),
            test_animation2->keyframe_effect()
                ->keyframe_models()[0]
                ->get_animation_data());

  auto id3 = lynx::tasm::CSSPropertyID::kPropertyIDColor;
  lynx::tasm::StyleMap output3;
  auto impl3 = lepus::Value("blue");
  lynx::tasm::UnitHandler::Process(id3, impl3, output3, configs);
  auto raw_value3 = output3[id3];
  auto test_value_pair3 = std::make_pair(id3, raw_value3);
  auto test_animation3 = InitTestAnimation();
  auto test_timing_function3 =
      animation::CubicBezierTimingFunction::MakeTimingFunction(
          test_animation3->animation_data()->timing_func);
  auto test_type3 = animation::AnimationCurve::CurveType::TEXTCOLOR;
  bool init_success3 = test_manager->InitCurveAndModelAndKeyframe(
      test_type3, test_animation3.get(), test_offset,
      std::move(test_timing_function3), test_value_pair3);
  EXPECT_EQ(init_success3, true);
  EXPECT_EQ(*test_animation3->animation_data(),
            test_animation3->keyframe_effect()
                ->keyframe_models()[0]
                ->get_animation_data());

  auto id4 = lynx::tasm::CSSPropertyID::kPropertyIDColor;
  lynx::tasm::StyleMap output4;
  auto impl4 = lepus::Value("");
  lynx::tasm::UnitHandler::Process(id4, impl4, output4, configs);
  auto raw_value4 = output4[id4];
  auto test_value_pair4 = std::make_pair(id4, raw_value4);
  auto test_animation4 = InitTestAnimation();
  auto test_timing_function4 =
      animation::CubicBezierTimingFunction::MakeTimingFunction(
          test_animation4->animation_data()->timing_func);
  auto test_type4 = animation::AnimationCurve::CurveType::UNSUPPORT;
  bool init_success4 = test_manager->InitCurveAndModelAndKeyframe(
      test_type4, test_animation4.get(), test_offset,
      std::move(test_timing_function4), test_value_pair4);
  EXPECT_EQ(init_success4, false);
}

TEST_F(CSSKeyframeManagerTest, GetDefaultValue) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  auto default_value1 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kLeft);
  EXPECT_EQ(default_value1, tasm::CSSValue::Empty());

  auto default_value2 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kOpacity);
  EXPECT_EQ(
      default_value2,
      tasm::CSSValue(lepus_value(animation::OpacityKeyframe::kDefaultOpacity),
                     tasm::CSSValuePattern::NUMBER));

  auto default_value3 = test_manager->GetDefaultValue(
      starlight::AnimationPropertyType::kBackgroundColor);
  EXPECT_EQ(default_value3,
            tasm::CSSValue(
                lepus_value(animation::ColorKeyframe::kDefaultBackgroundColor),
                tasm::CSSValuePattern::NUMBER));

  auto default_value4 =
      test_manager->GetDefaultValue(starlight::AnimationPropertyType::kNone);
  EXPECT_EQ(default_value4, tasm::CSSValue::Empty());
}

TEST_F(CSSKeyframeManagerTest, HasTwoSameAnimation) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  std::vector<starlight::AnimationData> animation_data;
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
  std::vector<starlight::AnimationData> animation_data;
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
  std::vector<starlight::AnimationData> animation_data;
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

TEST_F(CSSKeyframeManagerTest, UpdateAndFlushAnimatedStyle) {
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());

  auto id = lynx::tasm::CSSPropertyID::kPropertyIDLeft;
  lynx::tasm::StyleMap test_map;
  lynx::tasm::CSSParserConfigs configs;
  auto impl = lepus::Value("100px");
  lynx::tasm::UnitHandler::Process(id, impl, test_map, configs);
  auto final_map = test_element->final_animator_map_;

  bool update_flag = final_map.empty();
  EXPECT_TRUE(update_flag);

  bool flush_flag = test_element->FlushAnimatedStyle();
  EXPECT_FALSE(flush_flag);

  test_manager->UpdateFinalStyleMap(test_map);
  update_flag = test_element->final_animator_map_.empty();
  EXPECT_FALSE(update_flag);

  flush_flag = test_element->FlushAnimatedStyle();
  EXPECT_TRUE(flush_flag);
}

TEST_F(CSSKeyframeManagerTest, SetNeedsAnimationStyleRecalc) {
  auto test_animation = InitTestAnimation();
  auto test_element = InitElement();
  auto test_manager = InitTestKeyframeManager(test_element.get());
  test_manager->SetNeedsAnimationStyleRecalc(test_animation->name());
  auto final_map = test_element->final_animator_map_;
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
          {tasm::kPropertyIDTransform,
           starlight::AnimationPropertyType::kTransform},
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
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
