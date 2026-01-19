// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/style_resolver.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/simple_styling/simple_style_node.h"
#include "core/renderer/simple_styling/style_object.h"
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

// Mock implementation of SimpleStyleNode for testing
class MockSimpleStyleNode : public lynx::style::SimpleStyleNode {
 public:
  MockSimpleStyleNode() = default;
  ~MockSimpleStyleNode() override = default;

  void SetStyleObjects(std::unique_ptr<lynx::style::StyleObject*,
                                       lynx::style::StyleObjectArrayDeleter>
                           style_object) override {
    // Not needed for this test
  }

  void UpdateSimpleStyles(const tasm::StyleMap& style_map) override {
    current_styles_ = style_map;
  }

  void UpdateSimpleStyles(tasm::StyleMap&& style_map) override {
    current_styles_ = std::move(style_map);
  }

  void ResetSimpleStyle(tasm::CSSPropertyID id) override {
    current_styles_.erase(id);
  }

  // Helper method to get current styles for verification
  const tasm::StyleMap& GetCurrentStyles() const { return current_styles_; }

  // Helper method to check if a property exists
  bool HasProperty(tasm::CSSPropertyID id) const {
    return current_styles_.find(id) != current_styles_.end();
  }

 private:
  tasm::StyleMap current_styles_;
};

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
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .text-c
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("18px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

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
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

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
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

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
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

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
  fiber_element->style_resolver_.ResolveStyle(result, &indexFragment,
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
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .a
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("18px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

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
  fiber_element->style_resolver_.ResolveStyle(result, &indexFragment,
                                              &changed_css_vars);

  // check get the correct font-size
  const auto& value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(value.AsNumber() == 18);

  // class .a.b
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    // the key encoded as .b.a
    std::string key = ".b.a";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment.cascade_map_.emplace(key, tokens);
  }

  fiber_element->style_resolver_.ResolveStyle(result, &indexFragment,
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
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  CSSParserTokenMap indexTokensMap;
  // class .a
  {
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("18px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

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
  fiber_element->style_resolver_.ResolveStyle(result, &indexFragment,
                                              &changed_css_vars);

  // check get the correct font-size
  const auto& value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(value.AsNumber() == 18);

  // class .a.b
  {
    auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("20px");
    tokens.get()->raw_attributes_[id] = CSSValue(impl, CSSValuePattern::STRING);

    // the key encoded as .b.a
    std::string key = ".b.a";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);
    indexFragment.cascade_map_.emplace(key, tokens);
  }

  fiber_element->style_resolver_.ResolveStyle(result, &indexFragment,
                                              &changed_css_vars);

  // check get the correct font-size
  auto new_value = result.at(CSSPropertyID::kPropertyIDFontSize);

  EXPECT_TRUE(new_value.GetPattern() == CSSValuePattern::PX);
  EXPECT_TRUE(new_value.AsNumber() == 18);

  config->SetRemoveDescendantSelectorScope(true);

  fiber_element->style_resolver_.ResolveStyle(result, &indexFragment,
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
  auto token = fml::MakeRefCounted<CSSParseToken>(configs);
  token.get()->raw_attributes_[key] = CSSValue::MakePlainString("20px");

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

  fragment.rule_set()->AddStyleRule(fml::MakeRefCounted<StyleRule>(
      std::move(selector_array), std::move(token)));
  StyleMap result;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment, nullptr);

  auto value = result.at(key);
  // Can not match the selector
  EXPECT_NE(value.GetPattern(), CSSValuePattern::PX);

  // After removing the scope of the descendant selector,
  // the element can match the selector
  config->SetRemoveDescendantSelectorScope(true);
  result.clear();
  fiber_element->style_resolver_.ResolveStyle(result, &fragment, nullptr);
  auto new_value = result.at(key);
  EXPECT_EQ(new_value.GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(new_value.AsNumber(), 20);
}

TEST_F(CSSPatchingTest, ResolveStyleObjectsBasedOnExistingMap_EmptyOldAndNew) {
  // Test case: Both old and new style maps are empty
  tasm::StyleMap old_dcl_style;
  MockSimpleStyleNode target;
  target.UpdateSimpleStyles(old_dcl_style);

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(old_dcl_style, nullptr,
                                                 &target);

  EXPECT_TRUE(target.GetCurrentStyles().empty());
}

TEST_F(CSSPatchingTest, ResolveStyleObjectsBasedOnExistingMap_OnlyOldStyles) {
  // Test case: Only old styles exist, new styles are null
  tasm::StyleMap old_dcl_style;
  old_dcl_style[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("16px");
  old_dcl_style[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("red");

  MockSimpleStyleNode target;
  target.UpdateSimpleStyles(old_dcl_style);

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(old_dcl_style, nullptr,
                                                 &target);

  // All old styles should be reset since new styles are null
  EXPECT_TRUE(target.GetCurrentStyles().empty());
}

TEST_F(CSSPatchingTest, ResolveStyleObjectsBasedOnExistingMap_OnlyNewStyles) {
  // Test case: Only new styles exist, old styles are empty
  tasm::StyleMap old_dcl_style;

  // Create new style objects
  tasm::StyleMap new_style_map1;
  new_style_map1[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("18px");

  tasm::StyleMap new_style_map2;
  new_style_map2[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("blue");
  new_style_map2[CSSPropertyID::kPropertyIDWidth] =
      CSSValue::MakePlainString("100px");

  auto style_obj1 =
      fml::MakeRefCounted<lynx::style::StyleObject>(new_style_map1);
  auto style_obj2 =
      fml::MakeRefCounted<lynx::style::StyleObject>(new_style_map2);

  lynx::style::StyleObject* new_ptr[] = {style_obj1.get(), style_obj2.get(),
                                         nullptr};

  MockSimpleStyleNode target;
  target.UpdateSimpleStyles(old_dcl_style);

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(old_dcl_style, new_ptr,
                                                 &target);

  // All new styles should be applied
  EXPECT_TRUE(target.HasProperty(CSSPropertyID::kPropertyIDFontSize));
  EXPECT_TRUE(target.HasProperty(CSSPropertyID::kPropertyIDColor));
  EXPECT_TRUE(target.HasProperty(CSSPropertyID::kPropertyIDWidth));
  EXPECT_EQ(target.GetCurrentStyles().size(), 3u);
}

TEST_F(CSSPatchingTest,
       ResolveStyleObjectsBasedOnExistingMap_OverlappingStyles) {
  // Test case: Both old and new styles exist with some overlapping properties
  tasm::StyleMap old_dcl_style;
  old_dcl_style[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("16px");
  old_dcl_style[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("red");
  old_dcl_style[CSSPropertyID::kPropertyIDHeight] =
      CSSValue::MakePlainString("50px");

  // Create new style objects - font-size overlaps, color is new, height is not
  // in new styles
  tasm::StyleMap new_style_map1;
  new_style_map1[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("18px");  // Overrides old

  tasm::StyleMap new_style_map2;
  new_style_map2[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("blue");  // Updates existing
  new_style_map2[CSSPropertyID::kPropertyIDWidth] =
      CSSValue::MakePlainString("100px");  // New property

  auto style_obj1 =
      fml::MakeRefCounted<lynx::style::StyleObject>(new_style_map1);
  auto style_obj2 =
      fml::MakeRefCounted<lynx::style::StyleObject>(new_style_map2);

  lynx::style::StyleObject* new_ptr[] = {style_obj1.get(), style_obj2.get(),
                                         nullptr};

  MockSimpleStyleNode target;
  target.UpdateSimpleStyles(old_dcl_style);

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(old_dcl_style, new_ptr,
                                                 &target);

  // Check that overlapping properties are updated, new properties are added,
  // and old ones are reset
  EXPECT_TRUE(target.HasProperty(CSSPropertyID::kPropertyIDFontSize));
  EXPECT_TRUE(target.HasProperty(CSSPropertyID::kPropertyIDColor));
  EXPECT_TRUE(target.HasProperty(CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(
      target.HasProperty(CSSPropertyID::kPropertyIDHeight));  // Should be reset
  EXPECT_EQ(target.GetCurrentStyles().size(), 3u);
}

TEST_F(CSSPatchingTest, ResolveStyleObjectsBasedOnExistingMap_EmptyNewStyles) {
  // Test case: Old styles exist, but new styles array is empty (not null)
  tasm::StyleMap old_dcl_style;
  old_dcl_style[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("16px");
  old_dcl_style[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("red");

  // Empty new styles array (nullptr terminated)
  lynx::style::StyleObject* new_ptr[] = {nullptr};

  MockSimpleStyleNode target;
  target.UpdateSimpleStyles(old_dcl_style);

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(old_dcl_style, new_ptr,
                                                 &target);

  // All old styles should be reset since new styles are empty
  EXPECT_TRUE(target.GetCurrentStyles().empty());
}
// Mock SharedCSSFragmentWrapper for testing adopted stylesheets
class MockSharedCSSFragmentWrapper : public tasm::SharedCSSFragmentWrapper {
 public:
  MockSharedCSSFragmentWrapper() : SharedCSSFragmentWrapper(nullptr) {}
};

// Mock CSSFragment for testing adopted stylesheets
class MockCSSFragment : public tasm::SharedCSSFragment {
 public:
  MockCSSFragment() : SharedCSSFragment(-1, nullptr) {}

  bool enable_css_selector() override { return enable_css_selector_mock_; }

  void SetEnableCSSSelector(bool enable) {
    enable_css_selector_mock_ = enable;
    if (enable) {
      tasm::SharedCSSFragment::SetEnableCSSSelector();
    }
  }

 private:
  bool enable_css_selector_mock_ = true;
};

TEST_F(CSSPatchingTest, AdoptedStylesheets_MergeLogic) {
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetIdSelector("view-id");

  // 1. Create adopted stylesheet with a low-specificity rule (tag selector)
  auto mock_fragment = std::make_unique<MockCSSFragment>();
  mock_fragment->SetEnableCSSSelector(true);

  CSSParserConfigs configs;
  auto adopted_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  auto adopted_id = CSSPropertyID::kPropertyIDFontSize;
  auto adopted_impl = lepus::Value(30.0);
  adopted_tokens.get()->raw_attributes_[adopted_id] =
      CSSValue(adopted_impl, CSSValuePattern::PX);

  // Add rule to adopted fragment: "view { font-size: 30px; }" (specificity:
  // 0,0,1)
  auto selector = std::make_unique<css::LynxCSSSelector[]>(1);
  selector[0].SetMatch(css::LynxCSSSelector::kTag);
  selector[0].SetValue("view");
  selector[0].SetLastInTagHistory(true);
  selector[0].SetLastInSelectorList(true);
  mock_fragment->AddStyleRule(std::move(selector), adopted_tokens);

  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  wrapper->fragment_ = std::move(mock_fragment);
  manager->AdoptStyleSheet(wrapper);

  // 2. Create regular fragment with a high-specificity rule (ID selector)
  auto regular_fragment = std::make_unique<MockCSSFragment>();
  regular_fragment->SetEnableCSSSelector(true);

  auto regular_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  auto regular_id = CSSPropertyID::kPropertyIDFontSize;
  auto regular_impl = lepus::Value(10.0);
  regular_tokens.get()->raw_attributes_[regular_id] =
      CSSValue(regular_impl, CSSValuePattern::PX);

  // Add rule to regular fragment: "#view-id { font-size: 10px; }" (specificity:
  // 1,0,0)
  auto regular_selector = std::make_unique<css::LynxCSSSelector[]>(1);
  regular_selector[0].SetMatch(css::LynxCSSSelector::kId);
  regular_selector[0].SetValue("view-id");
  regular_selector[0].SetLastInTagHistory(true);
  regular_selector[0].SetLastInSelectorList(true);
  regular_fragment->AddStyleRule(std::move(regular_selector), regular_tokens);

  // 3. Resolve style
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, regular_fragment.get(),
                                              &changed_css_vars);

  // 4. Verify cascade priority: adopted stylesheet should override regular
  // stylesheet, even if regular stylesheet has higher specificity
  auto it = result.find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(it != result.end());
  EXPECT_EQ(it->second.AsNumber(), 30.0);
}

TEST_F(CSSPatchingTest, AdoptedStylesheets_BasicIntegration) {
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));

  auto mock_fragment = std::make_unique<MockCSSFragment>();
  mock_fragment->SetEnableCSSSelector(true);

  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  wrapper->fragment_ = std::move(mock_fragment);

  manager->AdoptStyleSheet(wrapper);

  EXPECT_EQ(manager->GetAdoptedStyleSheets().size(), 1);

  auto regular_fragment = std::make_unique<MockCSSFragment>();
  regular_fragment->SetEnableCSSSelector(true);

  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, regular_fragment.get(),
                                              &changed_css_vars);

  SUCCEED();
}

TEST_F(CSSPatchingTest, AdoptedStylesheets_EmptyList) {
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));

  EXPECT_TRUE(manager->GetAdoptedStyleSheets().empty());

  auto regular_fragment = std::make_unique<MockCSSFragment>();
  regular_fragment->SetEnableCSSSelector(true);

  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, regular_fragment.get(),
                                              &changed_css_vars);

  SUCCEED();
}

TEST_F(CSSPatchingTest, AdoptedStylesheets_DisabledSelector) {
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));

  auto mock_fragment = std::make_unique<MockCSSFragment>();
  mock_fragment->SetEnableCSSSelector(false);

  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  wrapper->fragment_ = std::move(mock_fragment);

  manager->AdoptStyleSheet(wrapper);

  auto regular_fragment = std::make_unique<MockCSSFragment>();
  regular_fragment->SetEnableCSSSelector(true);

  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, regular_fragment.get(),
                                              &changed_css_vars);

  SUCCEED();
}

TEST_F(CSSPatchingTest, GetCSSStyleNew_NoAdoptedStylesheets) {
  // Test that style resolution works when no adopted stylesheets are present
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  // Create a regular stylesheet fragment
  CSSParserConfigs configs;
  CSSParserTokenMap regular_css_map;
  auto regular_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  auto regular_id = CSSPropertyID::kPropertyIDFontSize;
  auto regular_impl = lepus::Value("16px");
  regular_tokens.get()->raw_attributes_[regular_id] =
      CSSValue(regular_impl, CSSValuePattern::STRING);

  std::string regular_key = ".test-class";
  auto& regular_sheets = regular_tokens->sheets();
  auto regular_shared_css_sheet = std::make_shared<CSSSheet>(regular_key);
  regular_sheets.emplace_back(regular_shared_css_sheet);
  regular_css_map.insert(std::make_pair(regular_key, regular_tokens));

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto regular_fragment = std::make_unique<MockCSSFragment>();
  regular_fragment->SetEnableCSSSelector(true);

  auto selector = std::make_unique<css::LynxCSSSelector[]>(1);
  selector[0].SetMatch(css::LynxCSSSelector::kClass);
  selector[0].SetValue("test-class");
  selector[0].SetLastInTagHistory(true);
  selector[0].SetLastInSelectorList(true);
  regular_fragment->AddStyleRule(std::move(selector), regular_tokens);

  // Resolve styles
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, regular_fragment.get(),
                                              &changed_css_vars);

  // Should get regular styles
  EXPECT_TRUE(result.find(CSSPropertyID::kPropertyIDFontSize) != result.end());
}

TEST_F(CSSPatchingTest, GetCSSStyleNew_AdoptedStylesheetDisabledSelector) {
  // Test that adopted stylesheets with disabled selectors are skipped
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  // Create a mock CSS fragment with disabled selector
  auto mock_fragment = std::make_unique<MockCSSFragment>();
  mock_fragment->SetEnableCSSSelector(false);  // Disabled!

  // Create wrapper for adopted stylesheet
  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  wrapper->fragment_ = std::move(mock_fragment);

  // Adopt the stylesheet
  manager->AdoptStyleSheet(wrapper);

  // Create a regular stylesheet fragment
  CSSParserConfigs configs;
  CSSParserTokenMap regular_css_map;
  auto regular_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  auto regular_id = CSSPropertyID::kPropertyIDFontSize;
  auto regular_impl = lepus::Value("16px");
  regular_tokens.get()->raw_attributes_[regular_id] =
      CSSValue(regular_impl, CSSValuePattern::STRING);

  std::string regular_key = ".test-class";
  auto& regular_sheets = regular_tokens->sheets();
  auto regular_shared_css_sheet = std::make_shared<CSSSheet>(regular_key);
  regular_sheets.emplace_back(regular_shared_css_sheet);
  regular_css_map.insert(std::make_pair(regular_key, regular_tokens));

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto regular_fragment = std::make_unique<MockCSSFragment>();
  regular_fragment->SetEnableCSSSelector(true);

  auto selector = std::make_unique<css::LynxCSSSelector[]>(1);
  selector[0].SetMatch(css::LynxCSSSelector::kClass);
  selector[0].SetValue("test-class");
  selector[0].SetLastInTagHistory(true);
  selector[0].SetLastInSelectorList(true);
  regular_fragment->AddStyleRule(std::move(selector), regular_tokens);

  // Resolve styles
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, regular_fragment.get(),
                                              &changed_css_vars);

  // Should still get regular styles since adopted stylesheet has disabled
  // selector
  EXPECT_TRUE(result.find(CSSPropertyID::kPropertyIDFontSize) != result.end());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
