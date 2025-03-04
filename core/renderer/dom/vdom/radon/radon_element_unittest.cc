// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/vdom/radon/radon_element.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/starlight/types/layout_attribute.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/template_codec/generator/ttml_holder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class RadonElementTest : public ::testing::Test {
 public:
  RadonElementTest() {}
  ~RadonElementTest() override {}
  lynx::tasm::ElementManager* manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto unique_manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    manager = unique_manager.get();
    tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
        *tasm_mediator.get(), std::move(unique_manager), 0);

    auto test_entry = std::make_shared<TemplateEntry>();
    tasm->template_entries_.insert({"test_entry", test_entry});

    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
};

TEST_F(RadonElementTest, CheckSetStyleInternal) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableComponentLayoutOnly(true);
  config->SetDefaultOverflowVisible(false);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);
  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));

  auto element = manager->CreateNode("view", comp.get()->attribute_holder());

  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(lepus::Value("30px"), lynx::tasm::CSSValuePattern::STRING),
      false);
  EXPECT_TRUE(element->has_layout_only_props_ == true);

  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDFilter,
      tasm::CSSValue(lepus::Value("grey"), lynx::tasm::CSSValuePattern::STRING),
      false);
  EXPECT_TRUE(element->has_layout_only_props_ == false);

  EXPECT_TRUE(element->has_transition_props_changed_ == false);
  EXPECT_TRUE(element->has_keyframe_props_changed_ == false);
  EXPECT_TRUE(element->has_z_props_ == false);

  EXPECT_TRUE(element->overflow_ == Element::OVERFLOW_HIDDEN);
  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDOverflow,
      tasm::CSSValue(lepus::Value((int)starlight::OverflowType::kVisible),
                     lynx::tasm::CSSValuePattern::NUMBER),
      false);
  EXPECT_TRUE(element->overflow_ == Element::OVERFLOW_XY);
  EXPECT_TRUE(element->has_non_flatten_attrs_ == false);

  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDTransition,
      tasm::CSSValue(lepus::Value("test"), lynx::tasm::CSSValuePattern::STRING),
      false);
  EXPECT_TRUE(element->has_transition_props_changed_ == true);
  EXPECT_TRUE(element->has_non_flatten_attrs_ == true);

  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDAnimation,
      tasm::CSSValue(lepus::Value("test"), lynx::tasm::CSSValuePattern::STRING),
      false);

  EXPECT_TRUE(element->has_keyframe_props_changed_ == true);
  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(lepus::Value("3"), lynx::tasm::CSSValuePattern::STRING),
      false);
  EXPECT_TRUE(element->has_z_props_ == true);

  element->ResetStyleInternal(CSSPropertyID::kPropertyIDTransition);
  element->ResetStyleInternal(CSSPropertyID::kPropertyIDAnimation);
  EXPECT_TRUE(element->has_non_flatten_attrs_ == true);
}

TEST_F(RadonElementTest, GetCSSID) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableComponentLayoutOnly(true);
  config->SetDefaultOverflowVisible(false);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);
  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 123, nullptr, nullptr, 0, 0, 0));

  auto element = manager->CreateNode("view", comp.get()->attribute_holder());

  EXPECT_TRUE(element->GetCSSID() == 123);
}

TEST_F(RadonElementTest, CheckShouldFlushGestureDetector) {
  // Create a shared configuration for the page.
  auto config = std::make_shared<PageConfig>();
  config->SetEnableComponentLayoutOnly(true);
  config->SetDefaultOverflowVisible(false);
  config->SetEnableZIndex(true);
  config->SetEnableNewGesture(true);

  // Set the configuration for the manager.
  manager->SetConfig(config);

  // Create a shared RadonComponent.
  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));

  // Create a unique Element using the manager.
  auto element = manager->CreateNode("view", comp.get()->attribute_holder());

  // Set an internal style property for the element's width.
  element->SetStyleInternal(
      CSSPropertyID::kPropertyIDWidth,
      tasm::CSSValue(lepus::Value("40px"), lynx::tasm::CSSValuePattern::STRING),
      false);

  // Validate that the element has layout-only properties.
  EXPECT_TRUE(element->has_layout_only_props_ == true);

  // Validate that the manager has the "EnableNewGesture" option enabled.
  EXPECT_TRUE(manager->GetEnableNewGesture() == true);
}

TEST_F(RadonElementTest, TestImageTextFlatten) {
  // Create a shared configuration for the page.
  auto config = std::make_shared<PageConfig>();
  config->SetEnableComponentLayoutOnly(true);
  config->SetDefaultOverflowVisible(false);
  config->SetEnableZIndex(true);
  config->SetEnableNewGesture(true);

  // Set the configuration for the manager.
  manager->SetConfig(config);

  // Create a shared RadonComponent.
  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));

  // Create a unique Element using the manager.
  auto element0 = manager->CreateNode("view", comp.get()->attribute_holder());

  // Set an internal style property for the element's width.
  element0->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(lepus::Value(100), lynx::tasm::CSSValuePattern::NUMBER),
      false);

  EXPECT_TRUE(element0->has_z_props_ == true);
  EXPECT_TRUE(element0->is_view() == true);
  EXPECT_TRUE(element0->is_image() == false);
  EXPECT_TRUE(element0->is_text() == false);
  EXPECT_TRUE(element0->TendToFlatten() == false);

  auto element1 = manager->CreateNode("image", comp.get()->attribute_holder());

  // Set an internal style property for the element's width.
  element1->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(lepus::Value(100), lynx::tasm::CSSValuePattern::NUMBER),
      false);

  EXPECT_TRUE(element1->has_z_props_ == true);
  EXPECT_TRUE(element1->is_image() == true);
  EXPECT_TRUE(element1->is_text() == false);
  EXPECT_TRUE(element1->TendToFlatten() == true);

  auto element2 = manager->CreateNode("text", comp.get()->attribute_holder());

  // Set an internal style property for the element's width.
  element2->SetStyleInternal(
      CSSPropertyID::kPropertyIDZIndex,
      tasm::CSSValue(lepus::Value(100), lynx::tasm::CSSValuePattern::NUMBER),
      false);

  EXPECT_TRUE(element2->has_z_props_ == true);
  EXPECT_TRUE(element2->is_image() == false);
  EXPECT_TRUE(element2->is_text() == true);
  EXPECT_TRUE(element2->TendToFlatten() == true);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
