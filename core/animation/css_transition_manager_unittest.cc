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
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/nlength.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/style/animation_data.h"
#include "core/style/transition_data.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

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
    data.property = type;
    data.duration = duration;
    data.delay = delay;
    data.timing_func = timing_func;
    return data;
  }

  fml::RefPtr<Element> InitElement() {
    auto test_element = manager->CreateNode("view", nullptr);
    test_element->SetAttribute(base::String("enable-new-animator"),
                               lepus::Value("true"));
    return test_element;
  }
};

TEST_F(CSSTransitionManagerTest, setTransitionData) {
  auto test_element = InitElement();
  auto test_manager = InitTestTransitionManager(test_element.get());
  std::vector<starlight::TransitionData> transition_data;
  transition_data.emplace_back(
      InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 100,
                         starlight::TimingFunctionData()));
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
  transition_data.emplace_back(
      InitTransitionData(starlight::AnimationPropertyType::kAll, 3000, 500,
                         starlight::TimingFunctionData()));
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
  std::vector<starlight::TransitionData> transition_data;
  transition_data.emplace_back(
      InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 0,
                         starlight::TimingFunctionData()));
  test_manager->setTransitionData(transition_data);
  test_manager->element()->RecordElementPreviousStyle(
      tasm::kPropertyIDOpacity,
      tasm::CSSValue(lepus::Value(0.5), CSSValuePattern::NUMBER));
  test_manager->ConsumeCSSProperty(
      tasm::kPropertyIDOpacity,
      tasm::CSSValue(lepus::Value(1), CSSValuePattern::NUMBER));
  // Animation map check
  EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
  starlight::AnimationData& opacity_animation_data =
      test_manager->animations_map()[base::String("opacity")]
          ->get_animation_data();
  EXPECT_TRUE(opacity_animation_data.name.IsEqual("opacity"));
  EXPECT_TRUE(opacity_animation_data.duration == 2000);
  EXPECT_TRUE(opacity_animation_data.delay == 0);

  // Transition ALL Check
  transition_data.clear();
  transition_data.emplace_back(
      InitTransitionData(starlight::AnimationPropertyType::kAll, 3000, 0,
                         starlight::TimingFunctionData()));
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
  std::vector<starlight::TransitionData> transition_data;
  transition_data.emplace_back(
      InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 0,
                         starlight::TimingFunctionData()));
  transition_data.emplace_back(
      InitTransitionData(starlight::AnimationPropertyType::kOpacity, 3000, 100,
                         starlight::TimingFunctionData()));
  test_manager->setTransitionData(transition_data);
  test_manager->element()->RecordElementPreviousStyle(
      tasm::kPropertyIDOpacity,
      tasm::CSSValue(lepus::Value(0.5), CSSValuePattern::NUMBER));
  test_manager->ConsumeCSSProperty(
      tasm::kPropertyIDOpacity,
      tasm::CSSValue(lepus::Value(1), CSSValuePattern::NUMBER));
  // Animation map check
  EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
  starlight::AnimationData& opacity_animation_data =
      test_manager->animations_map()[base::String("opacity")]
          ->get_animation_data();
  EXPECT_TRUE(opacity_animation_data.name.IsEqual("opacity"));
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
    std::vector<starlight::TransitionData> transition_data;
    transition_data.emplace_back(
        InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 0,
                           starlight::TimingFunctionData()));
    test_manager->setTransitionData(transition_data);
    test_manager->element()->RecordElementPreviousStyle(
        tasm::kPropertyIDOpacity,
        tasm::CSSValue(lepus::Value(0.5), CSSValuePattern::NUMBER));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDOpacity,
        tasm::CSSValue(lepus::Value(1), CSSValuePattern::NUMBER));
    EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
    transition_data.clear();
    test_manager->setTransitionData(transition_data);
    EXPECT_TRUE(test_manager->GetClearEffectAnimationName() == "opacity");
  }

  {
    // #2
    auto test_element = InitElement();
    auto test_manager = InitTestTransitionManager(test_element.get());
    std::vector<starlight::TransitionData> transition_data;
    transition_data.emplace_back(
        InitTransitionData(starlight::AnimationPropertyType::kOpacity, 2000, 0,
                           starlight::TimingFunctionData()));
    test_manager->setTransitionData(transition_data);
    test_manager->element()->RecordElementPreviousStyle(
        tasm::kPropertyIDOpacity,
        tasm::CSSValue(lepus::Value(0.5), CSSValuePattern::NUMBER));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDOpacity,
        tasm::CSSValue(lepus::Value(1), CSSValuePattern::NUMBER));
    EXPECT_TRUE(test_manager->animations_map().count(base::String("opacity")));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDOpacity,
        tasm::CSSValue(lepus::Value(0.8), CSSValuePattern::NUMBER));
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
    std::vector<starlight::TransitionData> transition_data;
    transition_data.emplace_back(
        InitTransitionData(starlight::AnimationPropertyType::kLeft, 2000, 0,
                           starlight::TimingFunctionData()));
    test_manager->setTransitionData(transition_data);
    test_manager->element()->RecordElementPreviousStyle(
        tasm::kPropertyIDLeft,
        tasm::CSSValue(lepus::Value(0), CSSValuePattern::NUMBER));
    test_manager->ConsumeCSSProperty(
        tasm::kPropertyIDLeft,
        tasm::CSSValue(lepus::Value(1), CSSValuePattern::NUMBER));
    EXPECT_TRUE(test_manager->animations_map().count(base::String("left")));
    test_manager->ConsumeCSSProperty(tasm::kPropertyIDLeft,
                                     tasm::CSSValue::Empty());
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
