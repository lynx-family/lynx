// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/animation/css_transition_manager.h"

#include <memory>

#include "core/animation/animation.h"
#include "core/animation/keyframe_effect.h"
#include "core/animation/keyframe_model.h"
#include "core/animation/keyframed_animation_curve.h"
#include "core/animation/testing/mock_css_transition_manager.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "core/style/filter_data.h"
#include "core/style/transform_raw_data.h"
#include "core/style/transition_data.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

namespace {

CssMeasureContext MakeTransitionMeasureContext(
    float layouts_unit_per_px = kDefaultLayoutsUnitPerPx) {
  return CssMeasureContext(kWidth, layouts_unit_per_px,
                           kDefaultPhysicalPixelsPerLayoutUnit, 16.f, 16.f,
                           starlight::LayoutUnit(), starlight::LayoutUnit());
}

}  // namespace

class CSSTransitionManagerTest : public ::testing::Test {
 public:
  CSSTransitionManagerTest() {}
  ~CSSTransitionManagerTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;

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

  std::unique_ptr<animation::MockCSSTransitionManager>
  InitTestTransitionManager(tasm::Element* element) {
    auto test_manager =
        std::make_unique<animation::MockCSSTransitionManager>(element);
    return test_manager;
  }

  starlight::TransitionData InitTransitionData(
      starlight::AnimationPropertyType type, long duration, long delay,
      starlight::TimingFunctionData timing_func) {
    starlight::TransitionData data;
    data.properties.push_back(type);
    data.durations.push_back(duration);
    data.delays.push_back(delay);
    data.timing_funcs.push_back(timing_func);
    return data;
  }

  fml::RefPtr<Element> InitElement() {
    manager->SetEnableNewAnimatorRadon(true);
    auto test_element = manager->CreateFiberElement("view");
    return test_element;
  }
};

TEST_F(CSSTransitionManagerTest, ConvertsCanonicalScalarValuesForTransition) {
  auto length = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDWidth,
      starlight::CanonicalComputedValue::Length(
          starlight::NLength::MakeUnitNLength(20.f)),
      MakeTransitionMeasureContext());
  ASSERT_TRUE(length.has_value());
  EXPECT_TRUE(length->IsNumber());
  EXPECT_FLOAT_EQ(length->AsNumber(), 20.f);

  auto percent = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDWidth,
      starlight::CanonicalComputedValue::Length(
          starlight::NLength::MakePercentageNLength(25.f)),
      MakeTransitionMeasureContext());
  ASSERT_TRUE(percent.has_value());
  EXPECT_TRUE(percent->IsPercent());
  EXPECT_FLOAT_EQ(percent->AsNumber(), 25.f);

  auto fixed_calc = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDWidth,
      starlight::CanonicalComputedValue::Length(
          starlight::NLength::MakeCalcNLength(6.f)),
      MakeTransitionMeasureContext());
  ASSERT_TRUE(fixed_calc.has_value());
  EXPECT_TRUE(fixed_calc->IsNumber());
  EXPECT_FLOAT_EQ(fixed_calc->AsNumber(), 6.f);

  auto mixed_calc = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDWidth,
      starlight::CanonicalComputedValue::Length(
          starlight::NLength::MakeCalcNLength(6.f, 25.f)),
      MakeTransitionMeasureContext());
  EXPECT_FALSE(mixed_calc.has_value());

  auto border_width = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDBorderLeftWidth,
      starlight::CanonicalComputedValue::ResolvedLength(2.f),
      MakeTransitionMeasureContext());
  ASSERT_TRUE(border_width.has_value());
  EXPECT_TRUE(border_width->IsNumber());
  EXPECT_FLOAT_EQ(border_width->AsNumber(), 2.f);

  auto opacity = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDOpacity, starlight::CanonicalComputedValue::Number(0.5f),
      MakeTransitionMeasureContext());
  ASSERT_TRUE(opacity.has_value());
  EXPECT_TRUE(opacity->IsNumber());
  EXPECT_FLOAT_EQ(opacity->AsNumber(), 0.5f);

  auto background_color = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDBackgroundColor,
      starlight::CanonicalComputedValue::Color(0xFF0000FF),
      MakeTransitionMeasureContext());
  ASSERT_TRUE(background_color.has_value());
  EXPECT_TRUE(background_color->IsNumber());
  EXPECT_EQ(static_cast<uint32_t>(background_color->AsNumber()), 0xFF0000FFU);
}

TEST_F(CSSTransitionManagerTest,
       ConvertsCanonicalBackgroundPositionPercentForTransition) {
  starlight::CanonicalComputedValue::BackgroundPositionValue position;
  position.emplace_back(starlight::NLength::MakePercentageNLength(25.f));
  position.emplace_back(starlight::NLength::MakePercentageNLength(75.f));

  auto converted = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDBackgroundPosition,
      starlight::CanonicalComputedValue::BackgroundPosition(position),
      MakeTransitionMeasureContext());

  ASSERT_TRUE(converted.has_value());
  ASSERT_TRUE(converted->IsArray());
  auto outer = converted->GetArray();
  ASSERT_EQ(outer->size(), 1U);
  ASSERT_TRUE(outer->get(0).IsArray());
  auto inner = outer->get(0).Array();
  ASSERT_EQ(inner->size(), 4U);
  EXPECT_EQ(inner->get(0).Number(), static_cast<int>(CSSValuePattern::PERCENT));
  EXPECT_FLOAT_EQ(inner->get(1).Number(), 25.f);
  EXPECT_EQ(inner->get(2).Number(), static_cast<int>(CSSValuePattern::PERCENT));
  EXPECT_FLOAT_EQ(inner->get(3).Number(), 75.f);
}

TEST_F(CSSTransitionManagerTest,
       ConvertsEmptyCanonicalBackgroundPositionToDefaultForTransition) {
  starlight::CanonicalComputedValue::BackgroundPositionValue position;

  auto converted = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDBackgroundPosition,
      starlight::CanonicalComputedValue::BackgroundPosition(position),
      MakeTransitionMeasureContext());

  ASSERT_TRUE(converted.has_value());
  ASSERT_TRUE(converted->IsArray());
  auto outer = converted->GetArray();
  ASSERT_EQ(outer->size(), 1U);
  ASSERT_TRUE(outer->get(0).IsArray());
  auto inner = outer->get(0).Array();
  ASSERT_EQ(inner->size(), 4U);
  EXPECT_EQ(inner->get(0).Number(), static_cast<int>(CSSValuePattern::PERCENT));
  EXPECT_FLOAT_EQ(inner->get(1).Number(), 0.f);
  EXPECT_EQ(inner->get(2).Number(), static_cast<int>(CSSValuePattern::PERCENT));
  EXPECT_FLOAT_EQ(inner->get(3).Number(), 0.f);
}

TEST_F(CSSTransitionManagerTest,
       ConvertsCanonicalTransformOriginPercentForTransition) {
  auto converted = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDTransformOrigin,
      starlight::CanonicalComputedValue::TransformOrigin(
          starlight::TransformOriginData()),
      MakeTransitionMeasureContext());

  ASSERT_TRUE(converted.has_value());
  ASSERT_TRUE(converted->IsArray());
  auto array = converted->GetArray();
  ASSERT_EQ(array->size(), 4U);
  EXPECT_FLOAT_EQ(array->get(0).Number(), 50.f);
  EXPECT_EQ(array->get(1).Number(), static_cast<int>(CSSValuePattern::PERCENT));
  EXPECT_FLOAT_EQ(array->get(2).Number(), 50.f);
  EXPECT_EQ(array->get(3).Number(), static_cast<int>(CSSValuePattern::PERCENT));
}

TEST_F(CSSTransitionManagerTest, ConvertsCanonicalTransformForTransition) {
  starlight::CanonicalComputedValue::TransformValue transform;
  starlight::TransformRawData translate;
  translate.type = starlight::TransformType::kTranslate;
  translate.p0 = starlight::NLength::MakeUnitNLength(12.f);
  translate.p1 = starlight::NLength::MakePercentageNLength(34.f);
  transform.emplace_back(translate);

  starlight::TransformRawData matrix;
  matrix.type = starlight::TransformType::kMatrix;
  matrix.matrix[0] = 1.0;
  matrix.matrix[1] = 2.0;
  matrix.matrix[4] = 3.0;
  matrix.matrix[5] = 4.0;
  matrix.matrix[12] = 10.0;
  matrix.matrix[13] = 20.0;
  transform.emplace_back(matrix);

  auto converted = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDTransform,
      starlight::CanonicalComputedValue::Transform(transform),
      MakeTransitionMeasureContext(2.f));

  ASSERT_TRUE(converted.has_value());
  ASSERT_TRUE(converted->IsArray());
  auto items = converted->GetArray();
  ASSERT_EQ(items->size(), 2U);

  ASSERT_TRUE(items->get(0).IsArray());
  auto translate_item = items->get(0).Array();
  ASSERT_EQ(translate_item->size(), 5U);
  EXPECT_EQ(translate_item->get(0).Number(),
            static_cast<int>(starlight::TransformType::kTranslate));
  EXPECT_FLOAT_EQ(translate_item->get(1).Number(), 12.f);
  EXPECT_EQ(translate_item->get(2).Number(),
            static_cast<int>(CSSValuePattern::NUMBER));
  EXPECT_FLOAT_EQ(translate_item->get(3).Number(), 34.f);
  EXPECT_EQ(translate_item->get(4).Number(),
            static_cast<int>(CSSValuePattern::PERCENT));

  ASSERT_TRUE(items->get(1).IsArray());
  auto matrix_item = items->get(1).Array();
  ASSERT_EQ(matrix_item->size(), 7U);
  EXPECT_EQ(matrix_item->get(0).Number(),
            static_cast<int>(starlight::TransformType::kMatrix));
  EXPECT_FLOAT_EQ(matrix_item->get(1).Number(), 1.0);
  EXPECT_FLOAT_EQ(matrix_item->get(2).Number(), 2.0);
  EXPECT_FLOAT_EQ(matrix_item->get(3).Number(), 3.0);
  EXPECT_FLOAT_EQ(matrix_item->get(4).Number(), 4.0);
  EXPECT_FLOAT_EQ(matrix_item->get(5).Number(), 5.0);
  EXPECT_FLOAT_EQ(matrix_item->get(6).Number(), 10.0);
}

TEST_F(CSSTransitionManagerTest, ConvertsCanonicalFilterForTransition) {
  auto none = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDFilter,
      starlight::CanonicalComputedValue::Filter(starlight::FilterData()),
      MakeTransitionMeasureContext());
  ASSERT_TRUE(none.has_value());
  EXPECT_TRUE(none->IsEmpty());

  starlight::FilterData blur;
  blur.type = starlight::FilterType::kBlur;
  blur.amount = starlight::NLength::MakeUnitNLength(20.f);
  auto converted = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDFilter, starlight::CanonicalComputedValue::Filter(blur),
      MakeTransitionMeasureContext());

  ASSERT_TRUE(converted.has_value());
  ASSERT_TRUE(converted->IsArray());
  auto array = converted->GetArray();
  ASSERT_EQ(array->size(), 3U);
  EXPECT_EQ(array->get(starlight::FilterData::kIndexType).Number(),
            static_cast<int>(starlight::FilterType::kBlur));
  EXPECT_FLOAT_EQ(array->get(starlight::FilterData::kIndexAmount).Number(),
                  20.f);
  EXPECT_EQ(array->get(starlight::FilterData::kIndexUnit).Number(),
            static_cast<int>(CSSValuePattern::NUMBER));

  starlight::ComputedCSSStyle round_trip_style{1.f, 1.f};
  ASSERT_TRUE(round_trip_style.SetValue(kPropertyIDFilter, *converted));
  ASSERT_TRUE(round_trip_style.GetFilterData());
  EXPECT_EQ(round_trip_style.GetFilterData()->type,
            starlight::FilterType::kBlur);
  EXPECT_EQ(round_trip_style.GetFilterData()->amount.GetType(),
            starlight::NLengthType::kNLengthUnit);
  EXPECT_FLOAT_EQ(round_trip_style.GetFilterData()->amount.GetRawValue(), 20.f);
}

TEST_F(CSSTransitionManagerTest, ConvertsCanonicalVisibilityEnumForTransition) {
  auto converted = animation::ConvertCanonicalComputedValueForTransition(
      kPropertyIDVisibility,
      starlight::CanonicalComputedValue::Enum(
          static_cast<int32_t>(starlight::VisibilityType::kHidden)),
      MakeTransitionMeasureContext());

  ASSERT_TRUE(converted.has_value());
  EXPECT_TRUE(converted->IsEnum());
  EXPECT_EQ(converted->AsNumber(),
            static_cast<int32_t>(starlight::VisibilityType::kHidden));
}

TEST_F(CSSTransitionManagerTest, setTransitionData) {
  auto test_element = InitElement();
  auto test_manager = InitTestTransitionManager(test_element.get());
  starlight::TransitionData transition_data =
      InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 100,
                         starlight::TimingFunctionData());
  test_manager->setTransitionData(transition_data);
  // animation data check
  EXPECT_TRUE(test_manager->animation_data().size() == 0);
  // transition data check
  EXPECT_TRUE(test_manager->transition_data().count(
      static_cast<unsigned int>(starlight::AnimationPropertyType::kOpacity)));

  // property_type_value check
  EXPECT_TRUE(test_manager->property_types().find(static_cast<unsigned int>(
                  starlight::AnimationPropertyType::kOpacity)) !=
              test_manager->property_types().end());

  // Transition ALL Check
  transition_data =
      InitTransitionData(starlight::AnimationPropertyType::kAll, 3000, 500,
                         starlight::TimingFunctionData());
  test_manager->setTransitionData(transition_data);
  // animation data check
  EXPECT_TRUE(test_manager->animation_data().size() == 0);
  // transition data check
  EXPECT_TRUE(test_manager->transition_data().count(
      static_cast<unsigned int>(starlight::AnimationPropertyType::kOpacity)));
  EXPECT_TRUE(test_manager->transition_data().size() ==
              animation::GetPropertyIDToAnimationPropertyTypeMap().size());
  const auto& transition_props_map =
      animation::GetPropertyIDToAnimationPropertyTypeMap();
  for (const auto& iterator : transition_props_map) {
    test_manager->property_types().emplace(
        static_cast<unsigned int>(iterator.second));
    auto& ani_data =
        test_manager
            ->transition_data()[static_cast<unsigned int>(iterator.second)];
    EXPECT_TRUE(ani_data.name.IsEqual(
        animation::ConvertAnimationPropertyTypeToString(iterator.second)));
    EXPECT_TRUE(ani_data.duration == 3000);
    EXPECT_TRUE(ani_data.delay == 500);
  }
  // property_type_value check
  EXPECT_TRUE(test_manager->property_types().size() ==
              transition_props_map.size());
}

TEST_F(CSSTransitionManagerTest, NoNeedUpdateExistingAnimator) {
  auto test_element = InitElement();
  auto test_manager = InitTestTransitionManager(test_element.get());
  starlight::TransitionData transition_data =
      InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 0,
                         starlight::TimingFunctionData());
  test_manager->setTransitionData(transition_data);
  test_manager->element()->RecordElementPreviousStyle(
      tasm::kPropertyIDOpacity, tasm::CSSValue(0.5, CSSValuePattern::NUMBER));
  test_manager->ConsumeCSSProperty(tasm::kPropertyIDOpacity,
                                   tasm::CSSValue(1, CSSValuePattern::NUMBER));
  // Animation map check
  EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
  starlight::AnimationData& opacity_animation_data =
      test_manager->animations_map()[base::String("opacity")]
          ->get_animation_data();
  EXPECT_TRUE(opacity_animation_data.name.IsEqual("opacity"));
  EXPECT_TRUE(opacity_animation_data.duration == 2000);
  EXPECT_TRUE(opacity_animation_data.delay == 0);

  // Transition ALL Check
  transition_data =
      InitTransitionData(starlight::AnimationPropertyType::kAll, 3000, 0,
                         starlight::TimingFunctionData());
  test_manager->setTransitionData(transition_data);
  // Animation map check
  EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
  opacity_animation_data =
      test_manager->animations_map()[base::String("opacity")]
          ->get_animation_data();
  EXPECT_TRUE(opacity_animation_data.name.IsEqual("opacity"));
  EXPECT_TRUE(opacity_animation_data.duration == 2000);
  EXPECT_TRUE(opacity_animation_data.delay == 0);
}

TEST_F(CSSTransitionManagerTest, HasTwoSameAnimation) {
  auto test_element = InitElement();
  auto test_manager = InitTestTransitionManager(test_element.get());
  // Create transition data with two opacity entries
  starlight::TransitionData transition_data;
  transition_data.properties.push_back(
      starlight::AnimationPropertyType::kOpacity);
  transition_data.durations.push_back(2000);
  transition_data.delays.push_back(0);
  transition_data.timing_funcs.emplace_back();

  // Second entry with same property
  transition_data.properties.push_back(
      starlight::AnimationPropertyType::kOpacity);
  transition_data.durations.push_back(3000);
  transition_data.delays.push_back(100);
  transition_data.timing_funcs.emplace_back();

  test_manager->setTransitionData(transition_data);
  test_manager->element()->RecordElementPreviousStyle(
      tasm::kPropertyIDOpacity, tasm::CSSValue(0.5, CSSValuePattern::NUMBER));
  test_manager->ConsumeCSSProperty(tasm::kPropertyIDOpacity,
                                   tasm::CSSValue(1, CSSValuePattern::NUMBER));
  // Animation map check
  EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
  starlight::AnimationData& opacity_animation_data =
      test_manager->animations_map()[base::String("opacity")]
          ->get_animation_data();
  EXPECT_TRUE(opacity_animation_data.name.IsEqual("opacity"));
  // Second entry wins
  EXPECT_TRUE(opacity_animation_data.duration == 3000);
  EXPECT_TRUE(opacity_animation_data.delay == 100);
  // transition data check
  EXPECT_TRUE(test_manager->transition_data().count(
      static_cast<unsigned int>(starlight::AnimationPropertyType::kOpacity)));
  auto& opacity_transition_data =
      test_manager->transition_data()[static_cast<unsigned int>(
          starlight::AnimationPropertyType::kOpacity)];
  EXPECT_TRUE(opacity_transition_data.name.IsEqual("opacity"));
  EXPECT_TRUE(opacity_transition_data.duration == 3000);
  EXPECT_TRUE(opacity_transition_data.delay == 100);
}

TEST_F(CSSTransitionManagerTest, ClearEffect) {
  {
    // #1
    auto test_element = InitElement();
    auto test_manager = InitTestTransitionManager(test_element.get());
    starlight::TransitionData transition_data =
        InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 0,
                           starlight::TimingFunctionData());
    test_manager->setTransitionData(transition_data);
    test_manager->element()->RecordElementPreviousStyle(
        tasm::kPropertyIDOpacity, tasm::CSSValue(0.5, CSSValuePattern::NUMBER));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDOpacity, tasm::CSSValue(1, CSSValuePattern::NUMBER));
    EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
    transition_data = starlight::TransitionData();
    test_manager->setTransitionData(transition_data);
    EXPECT_TRUE(test_manager->GetClearEffectAnimationName() == "opacity");
  }

  {
    // #2
    auto test_element = InitElement();
    auto test_manager = InitTestTransitionManager(test_element.get());
    starlight::TransitionData transition_data =
        InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 0,
                           starlight::TimingFunctionData());
    test_manager->setTransitionData(transition_data);
    test_manager->element()->RecordElementPreviousStyle(
        tasm::kPropertyIDOpacity, tasm::CSSValue(0.5, CSSValuePattern::NUMBER));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDOpacity, tasm::CSSValue(1, CSSValuePattern::NUMBER));
    EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDOpacity, tasm::CSSValue(0.8, CSSValuePattern::NUMBER));
    EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
    // If a transition animation is replaced by another identical transition
    // animation (both animate the same properties), then this transition
    // animation does not require applying the end effect.
    EXPECT_TRUE(test_manager->GetClearEffectAnimationName().empty());
  }

  {
    // #3
    auto test_element = InitElement();
    auto test_manager = InitTestTransitionManager(test_element.get());
    starlight::TransitionData transition_data =
        InitTransitionData(starlight::AnimationPropertyType::kLeft, 2000, 0,
                           starlight::TimingFunctionData());
    test_manager->setTransitionData(transition_data);
    test_manager->element()->RecordElementPreviousStyle(
        tasm::kPropertyIDLeft, tasm::CSSValue(0, CSSValuePattern::NUMBER));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDLeft, tasm::CSSValue(1, CSSValuePattern::NUMBER));
    EXPECT_TRUE(test_manager->animations_map().count(base::String("left")));
    test_manager->ConsumeCSSProperty(tasm::kPropertyIDLeft, tasm::CSSValue());
    EXPECT_TRUE(!test_manager->animations_map().count(base::String("left")));
    // If a transition animation is replaced by another identical transition
    // animation (both animate the same properties), then this transition
    // animation does not require applying the end effect.
    EXPECT_TRUE(test_manager->GetClearEffectAnimationName() == "left");
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
