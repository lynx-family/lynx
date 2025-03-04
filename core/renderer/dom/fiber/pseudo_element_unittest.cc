// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/fiber/pseudo_element.h"

#include "core/renderer/css/css_value.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
static constexpr double kEpsilon = 1e-6;

class PseudoElementTest : public ::testing::Test {
 public:
  PseudoElementTest() {}
  ~PseudoElementTest() override {}
  lynx::tasm::ElementManager *manager;
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

TEST_F(PseudoElementTest, Constructor) {
  PseudoState state = kPseudoStateSelection;
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  auto text_element = static_cast<lynx::tasm::FiberElement *>(text.get());
  PseudoElement pseudo_element(state, text_element);

  EXPECT_EQ(pseudo_element.state_, state);
  EXPECT_EQ(pseudo_element.holder_element_, text_element);
  EXPECT_TRUE(pseudo_element.ComputedCSSStyle() != nullptr);
  EXPECT_TRUE(pseudo_element.style_map_.empty());
}

TEST_F(PseudoElementTest, UpdateStyleMap) {
  PseudoState state = kPseudoStateSelection;
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  auto text_element = static_cast<lynx::tasm::FiberElement *>(text.get());
  PseudoElement pseudo_element(state, text_element);

  StyleMap new_style_map;
  new_style_map.insert_or_assign(
      kPropertyIDBackgroundColor,
      tasm::CSSValue(lepus::Value("red"), lynx::tasm::CSSValuePattern::STRING));
  pseudo_element.UpdateStyleMap(new_style_map);
  EXPECT_TRUE(pseudo_element.style_map_.size() == 1);
  EXPECT_TRUE(pseudo_element.style_map_.find(kPropertyIDBackgroundColor) !=
              pseudo_element.style_map_.end());

  new_style_map.clear();
  tasm::CSSValue new_background_color_value(
      lepus::Value("blue"), lynx::tasm::CSSValuePattern::STRING);
  new_style_map.insert_or_assign(kPropertyIDBackgroundColor,
                                 new_background_color_value);
  new_style_map.insert_or_assign(
      kPropertyIDColor, tasm::CSSValue(lepus::Value("green"),
                                       lynx::tasm::CSSValuePattern::STRING));
  pseudo_element.UpdateStyleMap(new_style_map);
  EXPECT_TRUE(pseudo_element.style_map_.size() == 2);
  EXPECT_TRUE(
      pseudo_element.style_map_.find(kPropertyIDBackgroundColor)->second ==
      new_background_color_value);

  new_style_map.clear();
  pseudo_element.UpdateStyleMap(new_style_map);
  EXPECT_TRUE(pseudo_element.style_map_.empty());

  auto view = manager->CreateFiberView();
  auto view_element = static_cast<lynx::tasm::FiberElement *>(view.get());
  PseudoElement placeholder_pseudo_element(kPseudoStatePlaceHolder,
                                           view_element);
  new_style_map.insert_or_assign(kPropertyIDFontSize,
                                 tasm::CSSValue(lepus::Value("30px")));
  placeholder_pseudo_element.UpdateStyleMap(new_style_map);
  EXPECT_TRUE(placeholder_pseudo_element.style_map_.size() == 1);
  EXPECT_TRUE(placeholder_pseudo_element.style_map_.find(kPropertyIDFontSize) !=
              placeholder_pseudo_element.style_map_.end());

  new_style_map.clear();
  placeholder_pseudo_element.UpdateStyleMap(new_style_map);
  EXPECT_TRUE(placeholder_pseudo_element.style_map_.empty());
}

TEST_F(PseudoElementTest, FontSizeUnit) {
  PseudoState state = kPseudoStateSelection;
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableCSSInheritance(true);
  std::unordered_set<CSSPropertyID> list = {kPropertyIDFontSize};
  config->SetCustomCSSInheritList(std::move(list));
  manager->SetConfig(config);
  auto page = manager->CreateFiberPage("page", 11);
  auto text = manager->CreateFiberText("text");
  auto text_element = static_cast<lynx::tasm::FiberElement *>(text.get());
  PseudoElement pseudo_element(state, text_element);
  StyleMap new_style_map;
  new_style_map.insert_or_assign(
      kPropertyIDFontSize,
      tasm::CSSValue(lepus::Value(2), CSSValuePattern::EM));
  pseudo_element.UpdateStyleMap(new_style_map);
  EXPECT_NEAR(
      pseudo_element.ComputedCSSStyle()->GetValue(kPropertyIDFontSize).Double(),
      28.f, kEpsilon);

  pseudo_element.SetFontSize(30.f, 14.f);
  EXPECT_NEAR(
      pseudo_element.ComputedCSSStyle()->GetValue(kPropertyIDFontSize).Double(),
      60.f, kEpsilon);

  new_style_map.clear();
  new_style_map.insert_or_assign(
      kPropertyIDFontSize,
      tasm::CSSValue(lepus::Value(3), CSSValuePattern::REM));
  pseudo_element.UpdateStyleMap(new_style_map);
  EXPECT_NEAR(
      pseudo_element.ComputedCSSStyle()->GetValue(kPropertyIDFontSize).Double(),
      42.f, kEpsilon);
  pseudo_element.SetFontSize(30.f, 50.f);
  EXPECT_NEAR(
      pseudo_element.ComputedCSSStyle()->GetValue(kPropertyIDFontSize).Double(),
      150.f, kEpsilon);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
