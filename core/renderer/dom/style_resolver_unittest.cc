// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/style_resolver.h"

#include "base/include/auto_reset.h"
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
    ++update_calls_;
    static_styles_ = style_map;
    dynamic_styles_.clear();
    current_styles_ = static_styles_;
  }

  void UpdateSimpleStyles(tasm::StyleMap&& style_map) override {
    ++update_calls_;
    static_styles_ = std::move(style_map);
    dynamic_styles_.clear();
    current_styles_ = static_styles_;
  }

  void UpdateStaticAndDynamicSimpleStyles(
      tasm::StyleMap&& style_map, tasm::StyleMap&& dynamic_style_map) override {
    ++update_calls_;
    static_styles_ = std::move(style_map);
    dynamic_styles_ = std::move(dynamic_style_map);
    RebuildCurrentStyles();
  }

  void UpdateDynamicSimpleStyles(tasm::StyleMap&& style_map) override {
    ++update_calls_;
    dynamic_styles_ = std::move(style_map);
    RebuildCurrentStyles();
  }

  void ResetSimpleStyle(tasm::CSSPropertyID id,
                        const tasm::CSSValue& value) override {
    current_styles_.insert_or_assign(id, value);
  }

  void ResetSimpleStyle(tasm::CSSPropertyID id) override {
    current_styles_.erase(id);
  }

  void ResetUpdateCalls() { update_calls_ = 0; }
  int update_calls() const { return update_calls_; }

  // Helper method to get current styles for verification
  const tasm::StyleMap& GetCurrentStyles() const { return current_styles_; }

  // Helper method to check if a property exists
  bool HasProperty(tasm::CSSPropertyID id) const {
    return current_styles_.find(id) != current_styles_.end();
  }

 private:
  void RebuildCurrentStyles() {
    current_styles_ = static_styles_;
    for (const auto& [property_id, value] : dynamic_styles_) {
      if (value.IsEmpty()) {
        if (const auto it = static_styles_.find(property_id);
            it != static_styles_.end()) {
          current_styles_.insert_or_assign(property_id, it->second);
        } else {
          current_styles_.erase(property_id);
        }
        continue;
      }
      current_styles_.insert_or_assign(property_id, value);
    }
  }

  int update_calls_{0};
  tasm::StyleMap static_styles_;
  tasm::StyleMap dynamic_styles_;
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
  target.ResetUpdateCalls();

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(old_dcl_style, nullptr,
                                                 &target);

  // All old styles should be reset since new styles are null
  EXPECT_TRUE(target.GetCurrentStyles().empty());
  EXPECT_EQ(target.update_calls(), 1);
}

TEST_F(CSSPatchingTest,
       ResolveStyleObjectsBasedOnExistingMap_EmptyNewStyles_ShouldClearMap) {
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
  target.ResetUpdateCalls();

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(old_dcl_style, new_ptr,
                                                 &target);

  EXPECT_TRUE(target.GetCurrentStyles().empty());
  EXPECT_EQ(target.update_calls(), 1);
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

TEST_F(CSSPatchingTest,
       ResolveStyleObjectsBasedOnExistingMap_DynamicOnlyNoOpSkipsUpdate) {
  tasm::StyleMap old_static_style;
  tasm::StyleMap old_dynamic_style;
  old_dynamic_style[CSSPropertyID::kPropertyIDOpacity] =
      CSSValue(0.5, CSSValuePattern::NUMBER);

  auto dynamic_object =
      fml::MakeRefCounted<lynx::style::DynamicStyleObject>(old_dynamic_style);

  MockSimpleStyleNode target;
  tasm::StyleMap initial_dynamic_style = old_dynamic_style;
  target.UpdateStaticAndDynamicSimpleStyles({},
                                            std::move(initial_dynamic_style));
  target.ResetUpdateCalls();

  StyleResolver resolver;
  resolver.ResolveStyleObjectsBasedOnExistingMap(
      old_static_style, nullptr, &old_dynamic_style, dynamic_object.get(),
      /*static_dirty=*/false, /*dynamic_dirty=*/true, &target);

  EXPECT_EQ(target.update_calls(), 0);
  const auto& current_styles = target.GetCurrentStyles();
  auto it = current_styles.find(CSSPropertyID::kPropertyIDOpacity);
  ASSERT_TRUE(it != current_styles.end());
  EXPECT_DOUBLE_EQ(it->second.GetNumber(), 0.5);
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

TEST_F(CSSPatchingTest, Specificity_Prioritize_Cascade_Order) {
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetIdSelector("view-id");
  base::String class_name("view-class");
  ClassList class_list;
  class_list.emplace_back(class_name);
  attribute_holder->SetClasses(std::move(class_list));

  // 1. Create a stylesheet with multiple selectors matching the element
  auto mock_fragment = std::make_unique<MockCSSFragment>();
  mock_fragment->SetEnableCSSSelector(true);

  CSSParserConfigs configs;

  // Rule 1: "view" (specificity: 0,0,1)
  auto rule1_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  rule1_tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue(lepus::Value(10.0), CSSValuePattern::PX);
  auto rule1_selector = std::make_unique<css::LynxCSSSelector[]>(1);
  rule1_selector[0].SetMatch(css::LynxCSSSelector::kTag);
  rule1_selector[0].SetValue("view");
  rule1_selector[0].SetLastInTagHistory(true);
  rule1_selector[0].SetLastInSelectorList(true);
  mock_fragment->AddStyleRule(std::move(rule1_selector), rule1_tokens);

  // Rule 2: ".view-class" (specificity: 0,1,0)
  auto rule2_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  rule2_tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue(lepus::Value(20.0), CSSValuePattern::PX);
  auto rule2_selector = std::make_unique<css::LynxCSSSelector[]>(1);
  rule2_selector[0].SetMatch(css::LynxCSSSelector::kClass);
  rule2_selector[0].SetValue("view-class");
  rule2_selector[0].SetLastInTagHistory(true);
  rule2_selector[0].SetLastInSelectorList(true);
  mock_fragment->AddStyleRule(std::move(rule2_selector), rule2_tokens);

  // Rule 3: "#view-id" (specificity: 1,0,0)
  auto rule3_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  rule3_tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue(lepus::Value(30.0), CSSValuePattern::PX);
  auto rule3_selector = std::make_unique<css::LynxCSSSelector[]>(1);
  rule3_selector[0].SetMatch(css::LynxCSSSelector::kId);
  rule3_selector[0].SetValue("view-id");
  rule3_selector[0].SetLastInTagHistory(true);
  rule3_selector[0].SetLastInSelectorList(true);
  mock_fragment->AddStyleRule(std::move(rule3_selector), rule3_tokens);

  // 2. Resolve style
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, mock_fragment.get(),
                                              &changed_css_vars);

  // 3. Verify that the highest specificity rule (#view-id) wins
  auto it = result.find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(it != result.end());
  EXPECT_EQ(it->second.AsNumber(), 30.0);

  // 4. Now add another rule with same specificity but later in the cascade
  auto rule4_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  rule4_tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue(lepus::Value(40.0), CSSValuePattern::PX);
  auto rule4_selector = std::make_unique<css::LynxCSSSelector[]>(1);
  rule4_selector[0].SetMatch(css::LynxCSSSelector::kId);
  rule4_selector[0].SetValue("view-id");
  rule4_selector[0].SetLastInTagHistory(true);
  rule4_selector[0].SetLastInSelectorList(true);
  mock_fragment->AddStyleRule(std::move(rule4_selector), rule4_tokens);

  StyleMap result2;
  CSSVariableMap changed_css_vars2;
  fiber_element->style_resolver_.ResolveStyle(result2, mock_fragment.get(),
                                              &changed_css_vars2);

  // 5. Verify that the later rule with the same specificity wins
  auto it2 = result2.find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(it2 != result2.end());
  EXPECT_EQ(it2->second.AsNumber(), 40.0);
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

TEST_F(CSSPatchingTest,
       AdoptedStylesheets_CascadePriorityWithEqualSpecificity) {
  // Test that adopted stylesheets have higher cascade priority than base
  // stylesheets when specificity is equal. This verifies the fix where adopted
  // stylesheets now share the same level counter as base stylesheets.
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  // 1. Create base stylesheet with a class selector (.test-class)
  // Specificity: 0,1,0
  CSSParserConfigs configs;
  auto base_fragment = std::make_unique<MockCSSFragment>();
  base_fragment->SetEnableCSSSelector(true);

  auto base_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  base_tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue(lepus::Value(10.0), CSSValuePattern::PX);

  auto base_selector = std::make_unique<css::LynxCSSSelector[]>(1);
  base_selector[0].SetMatch(css::LynxCSSSelector::kClass);
  base_selector[0].SetValue("test-class");
  base_selector[0].SetLastInTagHistory(true);
  base_selector[0].SetLastInSelectorList(true);
  base_fragment->AddStyleRule(std::move(base_selector), base_tokens);

  // 2. Create adopted stylesheet with the same class selector (.test-class)
  // Specificity: 0,1,0 (equal to base)
  auto adopted_css_fragment = std::make_unique<MockCSSFragment>();
  adopted_css_fragment->SetEnableCSSSelector(true);

  auto adopted_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  adopted_tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue(lepus::Value(20.0), CSSValuePattern::PX);

  auto adopted_selector = std::make_unique<css::LynxCSSSelector[]>(1);
  adopted_selector[0].SetMatch(css::LynxCSSSelector::kClass);
  adopted_selector[0].SetValue("test-class");
  adopted_selector[0].SetLastInTagHistory(true);
  adopted_selector[0].SetLastInSelectorList(true);
  adopted_css_fragment->AddStyleRule(std::move(adopted_selector),
                                     adopted_tokens);

  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  wrapper->fragment_ = std::move(adopted_css_fragment);
  manager->AdoptStyleSheet(wrapper);

  // 3. Resolve style
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, base_fragment.get(),
                                              &changed_css_vars);

  // 4. Verify cascade priority: adopted stylesheet should win because it has
  // higher cascade priority (higher position value due to shared level counter)
  auto it = result.find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(it != result.end());
  EXPECT_EQ(it->second.AsNumber(), 20.0)
      << "Adopted stylesheet should override base stylesheet when specificity "
         "is equal";
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

TEST_F(CSSPatchingTest, DidCollectMatchedRules_BulkCSSVariableUpdate) {
  // Test that CSS variables from multiple matched rules are correctly merged
  // and passed to AttributeHolder using bulk UpdateCSSVariable.
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  CSSParserConfigs configs;
  CSSParserTokenMap css_map;

  // Create a CSS parse token with CSS variables
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);

  // Add CSS variable: --primary-color: blue
  CSSVariableMap style_vars1;
  style_vars1.insert_or_assign(base::String("--primary-color"),
                               base::String("blue"));
  tokens->SetStyleVariables(std::move(style_vars1));

  // Add style attribute
  tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("16px");

  std::string key = ".test-class";
  auto& sheets = tokens->sheets();
  auto shared_css_sheet = std::make_shared<CSSSheet>(key);
  sheets.emplace_back(shared_css_sheet);
  css_map.insert_or_assign(key, tokens);

  // Create second rule with additional CSS variables
  auto tokens2 = fml::MakeRefCounted<CSSParseToken>(configs);
  CSSVariableMap style_vars2;
  style_vars2.insert_or_assign(base::String("--secondary-color"),
                               base::String("red"));
  tokens2->SetStyleVariables(std::move(style_vars2));

  std::string key2 = "view";
  auto& sheets2 = tokens2->sheets();
  auto shared_css_sheet2 = std::make_shared<CSSSheet>(key2);
  sheets2.emplace_back(shared_css_sheet2);
  css_map.insert_or_assign(key2, tokens2);

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  // First resolution - both variables should be in changed_css_vars
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment,
                                              &changed_css_vars);

  // Verify both CSS variables are tracked as changed
  EXPECT_EQ(changed_css_vars.size(), 2u);
  EXPECT_TRUE(changed_css_vars.find(base::String("--primary-color")) !=
              changed_css_vars.end());
  EXPECT_TRUE(changed_css_vars.find(base::String("--secondary-color")) !=
              changed_css_vars.end());
  EXPECT_EQ(changed_css_vars[base::String("--primary-color")].str(), "blue");
  EXPECT_EQ(changed_css_vars[base::String("--secondary-color")].str(), "red");

  // Verify variables are stored in attribute_holder
  const auto& stored_vars = attribute_holder->css_variables_map();
  EXPECT_EQ(stored_vars.size(), 2u);
  EXPECT_EQ(stored_vars.find(base::String("--primary-color"))->second.str(),
            "blue");
  EXPECT_EQ(stored_vars.find(base::String("--secondary-color"))->second.str(),
            "red");

  // Second resolution with modified variable
  auto tokens3 = fml::MakeRefCounted<CSSParseToken>(configs);
  CSSVariableMap style_vars3;
  style_vars3.insert_or_assign(base::String("--primary-color"),
                               base::String("green"));
  tokens3->SetStyleVariables(std::move(style_vars3));

  CSSParserTokenMap css_map2;
  auto& sheets3 = tokens3->sheets();
  sheets3.emplace_back(shared_css_sheet);
  css_map2.insert_or_assign(key, tokens3);

  // Keep the view selector with same value
  css_map2.insert_or_assign(key2, tokens2);

  SharedCSSFragment fragment2(1, dependent_ids, css_map2, keyframes, fontfaces);

  CSSVariableMap changed_css_vars2;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment2,
                                              &changed_css_vars2);

  // Only --primary-color should be marked as changed (from blue to green)
  EXPECT_EQ(changed_css_vars2.size(), 1u);
  EXPECT_TRUE(changed_css_vars2.find(base::String("--primary-color")) !=
              changed_css_vars2.end());
  EXPECT_EQ(changed_css_vars2[base::String("--primary-color")].str(), "green");

  // Verify updated value is stored
  const auto& stored_vars2 = attribute_holder->css_variables_map();
  EXPECT_EQ(stored_vars2.find(base::String("--primary-color"))->second.str(),
            "green");
  EXPECT_EQ(stored_vars2.find(base::String("--secondary-color"))->second.str(),
            "red");
}

TEST_F(CSSPatchingTest, DidCollectMatchedRules_CSSVariableRemoval) {
  // Test that CSS variables are correctly tracked when removed
  // Note: This test requires CSS inline variables to be enabled for the
  // bulk update path that properly handles variable removal.
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  CSSParserConfigs configs;
  CSSParserTokenMap css_map;

  // Create a CSS parse token with CSS variables
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  CSSVariableMap style_vars;
  style_vars.insert_or_assign(base::String("--primary-color"),
                              base::String("blue"));
  style_vars.insert_or_assign(base::String("--secondary-color"),
                              base::String("red"));
  tokens->SetStyleVariables(std::move(style_vars));

  tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("16px");

  std::string key = ".test-class";
  auto& sheets = tokens->sheets();
  auto shared_css_sheet = std::make_shared<CSSSheet>(key);
  sheets.emplace_back(shared_css_sheet);
  css_map.insert_or_assign(key, tokens);

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  // First resolution
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment,
                                              &changed_css_vars);

  EXPECT_EQ(changed_css_vars.size(), 2u);

  // Second resolution without CSS variables (simulating removal)
  auto tokens2 = fml::MakeRefCounted<CSSParseToken>(configs);
  tokens2.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("18px");

  CSSParserTokenMap css_map2;
  auto& sheets2 = tokens2->sheets();
  sheets2.emplace_back(shared_css_sheet);
  css_map2.insert_or_assign(key, tokens2);

  SharedCSSFragment fragment2(1, dependent_ids, css_map2, keyframes, fontfaces);

  CSSVariableMap changed_css_vars2;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment2,
                                              &changed_css_vars2);

  // Both variables should be marked as removed (empty value)
  EXPECT_EQ(changed_css_vars2.size(), 2u);
  EXPECT_EQ(changed_css_vars2[base::String("--primary-color")].str(), "");
  EXPECT_EQ(changed_css_vars2[base::String("--secondary-color")].str(), "");

  // Verify variables are removed from attribute_holder
  const auto& stored_vars = attribute_holder->css_variables_map();
  EXPECT_TRUE(stored_vars.empty());
}

TEST_F(CSSPatchingTest, DidCollectMatchedRules_DuplicateKeyPrecedence) {
  // Test that when the same CSS variable is defined in multiple matched rules,
  // the later rule's value takes precedence (last-match-wins semantics).
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  CSSParserConfigs configs;
  CSSParserTokenMap css_map;

  // First rule: --primary-color: blue
  auto tokens1 = fml::MakeRefCounted<CSSParseToken>(configs);
  CSSVariableMap style_vars1;
  style_vars1.insert_or_assign(base::String("--primary-color"),
                               base::String("blue"));
  tokens1->SetStyleVariables(std::move(style_vars1));

  std::string key1 = "view";
  auto& sheets1 = tokens1->sheets();
  auto shared_css_sheet1 = std::make_shared<CSSSheet>(key1);
  sheets1.emplace_back(shared_css_sheet1);
  css_map.insert_or_assign(key1, tokens1);

  // Second rule (higher precedence): --primary-color: red
  auto tokens2 = fml::MakeRefCounted<CSSParseToken>(configs);
  CSSVariableMap style_vars2;
  style_vars2.insert_or_assign(base::String("--primary-color"),
                               base::String("red"));
  tokens2->SetStyleVariables(std::move(style_vars2));

  std::string key2 = ".test-class";
  auto& sheets2 = tokens2->sheets();
  auto shared_css_sheet2 = std::make_shared<CSSSheet>(key2);
  sheets2.emplace_back(shared_css_sheet2);
  css_map.insert_or_assign(key2, tokens2);

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment,
                                              &changed_css_vars);

  // The later rule (.test-class) should win
  EXPECT_EQ(changed_css_vars.size(), 1u);
  EXPECT_EQ(changed_css_vars[base::String("--primary-color")].str(), "red");

  const auto& stored_vars = attribute_holder->css_variables_map();
  EXPECT_EQ(stored_vars.size(), 1u);
  EXPECT_EQ(stored_vars.find(base::String("--primary-color"))->second.str(),
            "red");
}

TEST_F(CSSPatchingTest, DidCollectMatchedRules_NoChangeDiff) {
  // Test that resolving with the same CSS variables produces empty
  // changed_css_vars (no invalidation needed).
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  CSSParserConfigs configs;
  CSSParserTokenMap css_map;

  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  CSSVariableMap style_vars;
  style_vars.insert_or_assign(base::String("--primary-color"),
                              base::String("blue"));
  style_vars.insert_or_assign(base::String("--secondary-color"),
                              base::String("red"));
  tokens->SetStyleVariables(std::move(style_vars));

  tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("16px");

  std::string key = ".test-class";
  auto& sheets = tokens->sheets();
  auto shared_css_sheet = std::make_shared<CSSSheet>(key);
  sheets.emplace_back(shared_css_sheet);
  css_map.insert_or_assign(key, tokens);

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  // First resolution
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment,
                                              &changed_css_vars);
  EXPECT_EQ(changed_css_vars.size(), 2u);

  // Second resolution with identical variables
  CSSVariableMap changed_css_vars2;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment,
                                              &changed_css_vars2);

  // No changes should be detected
  EXPECT_TRUE(changed_css_vars2.empty());

  // Verify variables are still stored correctly
  const auto& stored_vars = attribute_holder->css_variables_map();
  EXPECT_EQ(stored_vars.size(), 2u);
  EXPECT_EQ(stored_vars.find(base::String("--primary-color"))->second.str(),
            "blue");
  EXPECT_EQ(stored_vars.find(base::String("--secondary-color"))->second.str(),
            "red");
}

TEST_F(CSSPatchingTest, DidCollectMatchedRules_NullChangedCssVars) {
  // Test that ResolveStyle works correctly when changed_css_vars is nullptr.
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  auto* attribute_holder = fiber_element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("test-class");

  CSSParserConfigs configs;
  CSSParserTokenMap css_map;

  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  CSSVariableMap style_vars;
  style_vars.insert_or_assign(base::String("--primary-color"),
                              base::String("blue"));
  tokens->SetStyleVariables(std::move(style_vars));

  tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDFontSize] =
      CSSValue::MakePlainString("16px");

  std::string key = ".test-class";
  auto& sheets = tokens->sheets();
  auto shared_css_sheet = std::make_shared<CSSSheet>(key);
  sheets.emplace_back(shared_css_sheet);
  css_map.insert_or_assign(key, tokens);

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  // Resolve with nullptr for changed_css_vars (should not crash)
  StyleMap result;
  fiber_element->style_resolver_.ResolveStyle(result, &fragment, nullptr);

  // Verify variables are still stored correctly
  const auto& stored_vars = attribute_holder->css_variables_map();
  EXPECT_EQ(stored_vars.size(), 1u);
  EXPECT_EQ(stored_vars.find(base::String("--primary-color"))->second.str(),
            "blue");
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
