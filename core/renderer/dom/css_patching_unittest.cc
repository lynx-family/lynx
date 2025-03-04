// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/css_patching.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

using namespace css;

class CSSPatchingTest : public ::testing::Test {
 public:
  CSSPatchingTest() {}
  ~CSSPatchingTest() override {}

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

  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
};

TEST_F(CSSPatchingTest, GetCSSStyleForFiber) {
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("text");
  attribute_holder->SetClass("text-c");
  attribute_holder->SetIdSelector("#text-id");

  // constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = std::make_shared<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .text-c
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("18px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    std::string key = ".text-c";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  //* selector
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    std::string key = "*";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // tag selector
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("21px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    std::string key = "text";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  // id selector
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("22px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    std::string key = "#text-id";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment indexFragment(1, dependent_ids, indexTokensMap, keyframes,
                                  fontfaces);

  // check the id selector has higher Priority
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->css_patching_.ResolveStyle(result, &indexFragment,
                                            &changed_css_vars);

  // check get the correct font-size
  const auto& value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(value.AsNumber() == 22);
}

TEST_F(CSSPatchingTest, GetCSSStyleForFiberDescendantSelector) {
  // parent
  auto parent_fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* parent_attribute_holder = parent_fiber_element->data_model();
  parent_attribute_holder->set_tag("view");
  parent_attribute_holder->SetClass("a");
  parent_attribute_holder->SetIdSelector("#a-id");

  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("b");
  attribute_holder->SetIdSelector("#b-id");

  parent_fiber_element->InsertNode(fiber_element);

  // styles for fiber_element
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = std::make_shared<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .a
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("18px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    std::string key = ".b";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment indexFragment(1, dependent_ids, indexTokensMap, keyframes,
                                  fontfaces);

  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->css_patching_.ResolveStyle(result, &indexFragment,
                                            &changed_css_vars);

  // check get the correct font-size
  const auto& value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(value.AsNumber() == 18);

  // class .a.b
  {
    auto tokens = std::make_shared<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    // the key encoded as .b.a
    std::string key = ".b.a";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment.cascade_map_.emplace(key, tokens);
  }

  fiber_element->css_patching_.ResolveStyle(result, &indexFragment,
                                            &changed_css_vars);

  // check get the correct font-size
  auto& new_value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(new_value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(new_value.AsNumber() == 20);
}

// test descendant selector scope
TEST_F(CSSPatchingTest, FiberDescendantSelectorScope) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetRemoveDescendantSelectorScope(false);
  manager->SetConfig(config);

  // parent
  auto parent_fiber_element = manager->CreateFiberView();
  auto* parent_attribute_holder = parent_fiber_element->data_model();
  parent_attribute_holder->set_tag("view");
  parent_attribute_holder->SetClass("a");
  parent_attribute_holder->SetIdSelector("#a-id");

  auto fiber_element = manager->CreateFiberView();
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("b");
  attribute_holder->SetIdSelector("#b-id");

  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  fiber_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(fiber_element);

  parent_fiber_element->InsertNode(comp);

  // styles for fiber_element
  //  constructor css fragment
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto tokens = std::make_shared<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .a
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("18px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    std::string key = ".b";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexTokensMap.insert(std::make_pair(key, tokens));
  }

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment indexFragment(1, dependent_ids, indexTokensMap, keyframes,
                                  fontfaces);

  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->css_patching_.ResolveStyle(result, &indexFragment,
                                            &changed_css_vars);

  // check get the correct font-size
  const auto& value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(value.AsNumber() == 18);

  // class .a.b
  {
    auto tokens = std::make_shared<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl);

    // the key encoded as .b.a
    std::string key = ".b.a";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment.cascade_map_.emplace(key, tokens);
  }

  fiber_element->css_patching_.ResolveStyle(result, &indexFragment,
                                            &changed_css_vars);

  // check get the correct font-size
  auto new_value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(new_value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(new_value.AsNumber() == 18);

  config->SetRemoveDescendantSelectorScope(true);

  fiber_element->css_patching_.ResolveStyle(result, &indexFragment,
                                            &changed_css_vars);

  // check get the correct font-size
  new_value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(new_value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(new_value.AsNumber() == 20);
}

TEST_F(CSSPatchingTest, CSSSelectorDescendantSelectorScope) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetRemoveDescendantSelectorScope(false);
  manager->SetConfig(config);

  // parent
  auto parent_fiber_element = manager->CreateFiberView();
  auto* parent_attribute_holder = parent_fiber_element->data_model();
  parent_attribute_holder->set_tag("view");
  parent_attribute_holder->SetClass("a");
  parent_attribute_holder->SetIdSelector("#a-id");

  auto fiber_element = manager->CreateFiberView();
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("b");
  attribute_holder->SetIdSelector("#b-id");

  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  fiber_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp->impl_id()));
  comp->InsertNode(fiber_element);

  parent_fiber_element->InsertNode(comp);

  constexpr CSSPropertyID key = CSSPropertyID::kPropertyIDWidth;
  CSSParserConfigs configs;
  auto token = std::make_shared<CSSParseToken>(configs);
  token.get()->raw_attributes_[key] = CSSValue(lepus::Value("20px"));

  SharedCSSFragment fragment;
  // Create RuleSet
  fragment.SetEnableCSSSelector();
  // Create descendant selector
  auto selector_array = std::make_unique<LynxCSSSelector[]>(2);
  selector_array[0].SetValue("b");
  selector_array[0].SetRelation(LynxCSSSelector::RelationType::kDescendant);
  selector_array[0].SetMatch(LynxCSSSelector::MatchType::kClass);
  selector_array[0].SetLastInTagHistory(false);
  selector_array[0].SetLastInSelectorList(false);
  selector_array[1].SetValue("a");
  selector_array[1].SetMatch(LynxCSSSelector::MatchType::kClass);
  selector_array[1].SetLastInTagHistory(true);
  selector_array[1].SetLastInSelectorList(true);

  fragment.rule_set()->AddStyleRule(
      std::make_unique<StyleRule>(std::move(selector_array), std::move(token)));
  StyleMap result;
  fiber_element->css_patching_.ResolveStyle(result, &fragment, nullptr);

  auto value = result.at(key);
  // Can not match the selector
  EXPECT_NE(value.GetPattern(), CSSValuePattern::PX);

  // After removing the scope of the descendant selector,
  // the element can match the selector
  config->SetRemoveDescendantSelectorScope(true);
  result.clear();
  fiber_element->css_patching_.ResolveStyle(result, &fragment, nullptr);
  auto new_value = result.at(key);
  EXPECT_EQ(new_value.GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(new_value.AsNumber(), 20);
}
}  // namespace testing
}  // namespace tasm
}  // namespace lynx
