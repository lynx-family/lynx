// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/style_resolver.h"

#include "base/include/auto_reset.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/text_element.h"
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

static CSSValue ParseVariableValue(const char* raw_value,
                                   const CSSParserConfigs& configs) {
  lepus::Value value(raw_value);
  CSSStringParser parser = CSSStringParser::FromLepusString(value, configs);
  return parser.ParseVariable();
}

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

  fml::RefPtr<PageElement> CreatePageRoot(double font_size) {
    auto page = manager->CreateFiberPage("page", 0);
    manager->SetRoot(page.get());
    page->computed_css_style()->SetFontSize(font_size, font_size);
    return page;
  }

  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
};

void ExpectPxStyle(const StyleMap& styles, CSSPropertyID id, double expected) {
  auto it = styles.find(id);
  ASSERT_TRUE(it != styles.end());
  EXPECT_EQ(it->second.GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(it->second.AsNumber(), expected);
}

void ExpectNumberStyle(const StyleMap& styles, CSSPropertyID id,
                       double expected) {
  auto it = styles.find(id);
  ASSERT_TRUE(it != styles.end());
  EXPECT_EQ(it->second.GetPattern(), CSSValuePattern::NUMBER);
  EXPECT_EQ(it->second.AsNumber(), expected);
}

void ExpectNoStyle(const StyleMap& styles, CSSPropertyID id) {
  EXPECT_TRUE(styles.find(id) == styles.end());
}

bool HasChanged(const starlight::ComputedCSSStyle& style, CSSPropertyID id) {
  return const_cast<starlight::ComputedCSSStyle&>(style).GetChangedBitset().Has(
      id);
}

void ExpectResolvedVariable(starlight::ComputedCSSStyle& style,
                            const char* name, const char* expected) {
  style.FinalizeCustomProperties();
  auto value = style.ResolveVariable(base::String(name));
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(value.value().AsStdString(), expected);
}

std::unique_ptr<SharedCSSFragment> MakeSelectorFragmentWithToken(
    const std::string& selector_text, fml::RefPtr<CSSParseToken> token) {
  auto fragment = std::make_unique<SharedCSSFragment>();
  fragment->SetEnableCSSSelector();

  css::CSSParserContext context;
  css::CSSTokenizer tokenizer(selector_text);
  const auto tokens = tokenizer.TokenizeToEOF();
  css::CSSParserTokenRange range(tokens);
  css::LynxCSSSelectorVector vector =
      css::CSSSelectorParser::ParseSelector(range, &context);
  size_t flattened_size = css::CSSSelectorParser::FlattenedSize(vector);
  auto selector_array =
      std::make_unique<css::LynxCSSSelector[]>(flattened_size);
  css::CSSSelectorParser::AdoptSelectorVector(vector, selector_array.get(),
                                              flattened_size);
  fragment->AddStyleRule(std::move(selector_array), std::move(token));
  return fragment;
}

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

  // 3. Wrap regular fragment in decorator so adopted sheets are visible
  CSSFragmentDecorator decorator(regular_fragment.get(), manager.get());

  // 4. Resolve style
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, &decorator,
                                              &changed_css_vars);

  // 5. Verify cascade priority: adopted stylesheet should override regular
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

  // 3. Wrap base fragment in decorator so adopted sheets participate
  CSSFragmentDecorator decorator(base_fragment.get(), manager.get());

  // 4. Resolve style
  StyleMap result;
  CSSVariableMap changed_css_vars;
  fiber_element->style_resolver_.ResolveStyle(result, &decorator,
                                              &changed_css_vars);

  // 5. Verify cascade priority: adopted stylesheet should win because it has
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

TEST_F(CSSPatchingTest, HandlePseudoElement_AdoptedStylesheetHasPseudoRules) {
  // Test that HandlePseudoElement does not bail out early when only adopted
  // stylesheets carry pseudo rules and the base fragment has none.
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("11", 20);
  manager->SetRoot(page.get());

  auto fiber_element = manager->CreateFiberView();
  fiber_element->has_placeholder_ = true;
  page->InsertNode(fiber_element);

  // Base fragment: enable CSS selector but NO pseudo rules.
  auto base_fragment = std::make_unique<MockCSSFragment>();
  base_fragment->SetEnableCSSSelector(true);
  // rule_set() exists but pseudo_rules() is empty.

  CSSParserConfigs configs;
  auto pseudo_tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  pseudo_tokens.get()->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("blue");

  auto adopted_fragment =
      MakeSelectorFragmentWithToken("view::placeholder", pseudo_tokens);

  // Sanity: adopted fragment now has pseudo rules, base has none.
  ASSERT_TRUE(base_fragment->rule_set()->pseudo_rules().empty());
  ASSERT_FALSE(adopted_fragment->rule_set()->pseudo_rules().empty());

  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  wrapper->fragment_ = std::move(adopted_fragment);
  manager->AdoptStyleSheet(wrapper);

  CSSFragmentDecorator decorator(base_fragment.get(), manager.get());
  ASSERT_TRUE(decorator.HasPseudoRules());
  fiber_element->style_resolver_.HandlePseudoElement(&decorator);

  ASSERT_TRUE(fiber_element->pseudo_elements_.has_value());
  auto placeholder_it =
      fiber_element->pseudo_elements_->find(kPseudoStatePlaceHolder);
  ASSERT_TRUE(placeholder_it != fiber_element->pseudo_elements_->end());
  EXPECT_TRUE(placeholder_it->second->style_map_.contains(kPropertyIDColor));
}

TEST_F(CSSPatchingTest,
       HandlePseudoElement_NoPseudoInBaseNorAdopted_EarlyReturn) {
  // When neither the base fragment nor adopted stylesheets have pseudo rules,
  // HandlePseudoElement should early-return without issue.
  auto fiber_element =
      fml::AdoptRef<FiberElement>(new FiberElement(manager.get(), "view"));
  fiber_element->data_model()->set_tag("view");

  auto base_fragment = std::make_unique<MockCSSFragment>();
  base_fragment->SetEnableCSSSelector(true);

  // No adopted stylesheets at all.
  EXPECT_TRUE(manager->GetAdoptedStyleSheets().empty());

  fiber_element->style_resolver_.HandlePseudoElement(base_fragment.get());
  SUCCEED();
}

TEST_F(CSSPatchingTest,
       NewStylingCreateInitialComputedStyleInitializesShellAndOverflowDefault) {
  constexpr double kRootFontSize = 28.0;
  manager->lynx_env_config_.viewport_height_ = kHeight;
  manager->lynx_env_config_.viewport_width_ = kWidth;
  auto page = CreatePageRoot(kRootFontSize);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle previous_style(*manager->platform_computed_css());
  previous_style.SetOverflowDefaultVisible(true);

  auto style = element->style_resolver_.CreateInitialComputedStyle(
      nullptr, &previous_style);
  ASSERT_NE(style, nullptr);

  EXPECT_EQ(style->GetMeasureContext().viewport_width_,
            starlight::LayoutUnit(kWidth));
  EXPECT_EQ(style->GetMeasureContext().viewport_height_,
            starlight::LayoutUnit(kHeight));
  EXPECT_EQ(style->GetMeasureContext().screen_width_, kWidth);
  EXPECT_EQ(style->GetMeasureContext().layouts_unit_per_px_,
            kDefaultLayoutsUnitPerPx);
  EXPECT_EQ(style->GetMeasureContext().physical_pixels_per_layout_unit_,
            kDefaultPhysicalPixelsPerLayoutUnit);
  EXPECT_DOUBLE_EQ(style->GetFontSize(),
                   manager->GetLynxEnvConfig().PageDefaultFontSize());
  EXPECT_DOUBLE_EQ(style->GetRootFontSize(), kRootFontSize);
  EXPECT_EQ(style->GetDefaultOverflowType(), starlight::OverflowType::kVisible);
  EXPECT_EQ(style->GetOverflow(), starlight::OverflowType::kVisible);
  EXPECT_TRUE(style->IsOverflowXY());
}

TEST_F(CSSPatchingTest,
       NewStylingCreateInitialComputedStyleWithNonDefaultFontScale) {
  constexpr double kRootFontSize = 28.0;
  constexpr float kFontScale = 1.5f;
  manager->lynx_env_config_.SetFontScale(kFontScale);
  manager->lynx_env_config_.viewport_height_ = kHeight;
  manager->lynx_env_config_.viewport_width_ = kWidth;
  auto page = CreatePageRoot(kRootFontSize);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  auto style =
      element->style_resolver_.CreateInitialComputedStyle(nullptr, nullptr);
  ASSERT_NE(style, nullptr);

  EXPECT_FLOAT_EQ(style->length_context_.font_scale_, kFontScale);
  EXPECT_DOUBLE_EQ(style->GetFontSize(),
                   manager->GetLynxEnvConfig().PageDefaultFontSize());
  EXPECT_DOUBLE_EQ(style->GetRootFontSize(), kRootFontSize);
}

TEST_F(CSSPatchingTest,
       NewStylingCreateInitialComputedStyleInheritsParentAndLiveRootFont) {
  constexpr double kParentFontSize = 20.0;
  constexpr double kStaleParentRootFontSize = 12.0;
  constexpr double kLiveRootFontSize = 32.0;
  manager->config_->SetEnableCSSInheritance(true);
  manager->config_->css_configs_.custom_inherit_list_.insert(
      CSSPropertyID::kPropertyIDDirection);

  auto page = CreatePageRoot(kLiveRootFontSize);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle parent_style(*manager->platform_computed_css());
  parent_style.SetFontSize(kParentFontSize, kStaleParentRootFontSize);
  parent_style.SetValue(CSSPropertyID::kPropertyIDDirection,
                        CSSValue(starlight::DirectionType::kRtl), false);
  parent_style.SetValue(CSSPropertyID::kPropertyIDOpacity,
                        CSSValue(0.3, CSSValuePattern::NUMBER), false);
  parent_style.SetCustomProperty(base::String("--accent"),
                                 CSSValue::MakePlainString("blue"));
  parent_style.FinalizeCustomProperties();

  auto style = element->style_resolver_.CreateInitialComputedStyle(
      &parent_style, nullptr);
  ASSERT_NE(style, nullptr);

  EXPECT_DOUBLE_EQ(style->GetFontSize(), kParentFontSize);
  EXPECT_DOUBLE_EQ(style->GetRootFontSize(), kLiveRootFontSize);

  auto inherited_custom = style->ResolveVariable(base::String("--accent"));
  ASSERT_TRUE(inherited_custom.has_value());
  EXPECT_EQ(inherited_custom.value().AsStdString(), "blue");

  const auto& resolved_values = style->GetResolvedValues();
  auto inherited_direction =
      resolved_values.find(CSSPropertyID::kPropertyIDDirection);
  ASSERT_TRUE(inherited_direction != resolved_values.end());
  EXPECT_EQ(inherited_direction->second,
            CSSValue(starlight::DirectionType::kRtl));
  EXPECT_FALSE(resolved_values.contains(CSSPropertyID::kPropertyIDOpacity));
}

TEST_F(CSSPatchingTest,
       NewStylingCreateInitialComputedStyleSkipsNormalInheritanceWhenDisabled) {
  constexpr double kParentFontSize = 20.0;
  constexpr double kParentRootFontSize = 12.0;
  constexpr double kLiveRootFontSize = 30.0;
  manager->config_->SetEnableCSSInheritance(false);

  auto page = CreatePageRoot(kLiveRootFontSize);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle parent_style(*manager->platform_computed_css());
  parent_style.SetFontSize(kParentFontSize, kParentRootFontSize);
  parent_style.SetValue(CSSPropertyID::kPropertyIDDirection,
                        CSSValue(starlight::DirectionType::kRtl), false);
  parent_style.SetCustomProperty(base::String("--accent"),
                                 CSSValue::MakePlainString("blue"));
  parent_style.FinalizeCustomProperties();

  auto style = element->style_resolver_.CreateInitialComputedStyle(
      &parent_style, nullptr);
  ASSERT_NE(style, nullptr);

  EXPECT_DOUBLE_EQ(style->GetFontSize(),
                   manager->GetLynxEnvConfig().PageDefaultFontSize());
  EXPECT_DOUBLE_EQ(style->GetRootFontSize(), kLiveRootFontSize);

  auto inherited_custom = style->ResolveVariable(base::String("--accent"));
  ASSERT_TRUE(inherited_custom.has_value());
  EXPECT_EQ(inherited_custom.value().AsStdString(), "blue");
  EXPECT_FALSE(
      style->GetResolvedValues().contains(CSSPropertyID::kPropertyIDDirection));
}

TEST_F(CSSPatchingTest,
       NewStylingPrepareInitialComputedStyleReusesCurrentShellInPlace) {
  constexpr double kParentFontSize = 18.0;
  constexpr double kParentRootFontSize = 16.0;
  constexpr double kLiveRootFontSize = 36.0;
  manager->config_->SetEnableCSSInheritance(true);

  auto page = CreatePageRoot(kLiveRootFontSize);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  style.SetViewportWidth(starlight::LayoutUnit(77));
  style.SetViewportHeight(starlight::LayoutUnit(88));
  style.SetFontSize(10.0, 10.0);

  starlight::ComputedCSSStyle parent_style(*manager->platform_computed_css());
  parent_style.SetFontSize(kParentFontSize, kParentRootFontSize);

  element->style_resolver_.PrepareInitialComputedStyle(style, &parent_style,
                                                       &style);

  EXPECT_EQ(style.GetMeasureContext().viewport_width_,
            starlight::LayoutUnit(77));
  EXPECT_EQ(style.GetMeasureContext().viewport_height_,
            starlight::LayoutUnit(88));
  EXPECT_DOUBLE_EQ(style.GetFontSize(), kParentFontSize);
  EXPECT_DOUBLE_EQ(style.GetRootFontSize(), kLiveRootFontSize);
}

TEST_F(CSSPatchingTest,
       NewStylingCollectMatchedRulesPopulatesTlsAndClearsOldState) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  auto* attribute_holder = element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("matched");

  StyleMap stale_matched_styles;
  stale_matched_styles.insert_or_assign(CSSPropertyID::kPropertyIDHeight,
                                        CSSValue(999, CSSValuePattern::NUMBER));
  StyleMap stale_important_styles;
  stale_important_styles.insert_or_assign(
      CSSPropertyID::kPropertyIDOpacity,
      CSSValue(0.5, CSSValuePattern::NUMBER));
  CSSVariableMap stale_variables;
  stale_variables.insert_or_assign(base::String("--stale"),
                                   base::String("orange"));
  StyleResolver::matched_style_map.push_back(&stale_matched_styles);
  StyleResolver::matched_important_style_map.push_back(&stale_important_styles);
  StyleResolver::matched_variable_map.push_back(&stale_variables);

  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
      CSSValue::MakePlainString("120px");
  tokens->raw_attributes_[CSSPropertyID::kPropertyIDMarginInlineStart] =
      CSSValue::MakePlainString("6px");
  CSSVariableMap matched_variables;
  matched_variables.insert_or_assign(base::String("--accent"),
                                     base::String("green"));
  tokens->SetStyleVariables(std::move(matched_variables));

  std::string key = ".matched";
  tokens->sheets().emplace_back(std::make_shared<CSSSheet>(key));
  CSSParserTokenMap css_map;
  css_map.insert_or_assign(key, tokens);
  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  element->style_resolver_.CollectMatchedRules(&fragment);
  EXPECT_TRUE(StyleResolver::matched_important_style_map.empty());
  ASSERT_EQ(StyleResolver::matched_style_map.size(), 1u);
  ASSERT_EQ(StyleResolver::matched_variable_map.size(), 1u);
  ExpectNoStyle(*StyleResolver::matched_style_map[0],
                CSSPropertyID::kPropertyIDHeight);
  EXPECT_TRUE(StyleResolver::matched_variable_map[0]->find(base::String(
                  "--stale")) == StyleResolver::matched_variable_map[0]->end());

  StyleResolver::matched_style_map.clear();
  StyleResolver::matched_important_style_map.clear();
  StyleResolver::matched_variable_map.clear();
}

TEST_F(CSSPatchingTest, NewStylingCollectMatchedRulesBuildsStaticInputs) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  auto* attribute_holder = element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("matched");

  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  tokens->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
      CSSValue::MakePlainString("120px");
  tokens->raw_attributes_[CSSPropertyID::kPropertyIDMarginInlineStart] =
      CSSValue::MakePlainString("6px");
  CSSVariableMap matched_variables;
  matched_variables.insert_or_assign(base::String("--accent"),
                                     base::String("green"));
  tokens->SetStyleVariables(std::move(matched_variables));

  std::string key = ".matched";
  tokens->sheets().emplace_back(std::make_shared<CSSSheet>(key));
  CSSParserTokenMap css_map;
  css_map.insert_or_assign(key, tokens);
  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  element->style_resolver_.CollectMatchedRules(&fragment);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  style.SetValue(CSSPropertyID::kPropertyIDDirection,
                 CSSValue(starlight::DirectionType::kRtl), false);
  StyleResolver::NewPipelineCollectedStyleInputs inputs;
  element->style_resolver_.CollectStaticStyleInputs(style, inputs, 0);

  ExpectPxStyle(inputs.matched_styles, CSSPropertyID::kPropertyIDWidth, 120);
  ExpectNoStyle(inputs.matched_styles,
                CSSPropertyID::kPropertyIDMarginInlineStart);
  ExpectNoStyle(inputs.matched_styles, CSSPropertyID::kPropertyIDMarginLeft);
  ExpectPxStyle(inputs.matched_styles, CSSPropertyID::kPropertyIDMarginRight,
                6);
  ExpectResolvedVariable(style, "--accent", "green");
}

TEST_F(CSSPatchingTest,
       NewStylingCollectMatchedRulesSupportsSelectorFragments) {
  auto element = manager->CreateFiberView();
  auto* attribute_holder = element->data_model();
  attribute_holder->set_tag("view");
  attribute_holder->SetClass("matched");

  CSSParserConfigs configs;
  auto token = fml::MakeRefCounted<CSSParseToken>(configs);
  token->raw_attributes_[CSSPropertyID::kPropertyIDWidth] =
      CSSValue::MakePlainString("120px");
  token->raw_important_attributes_[CSSPropertyID::kPropertyIDHeight] =
      CSSValue::MakePlainString("24px");
  CSSVariableMap matched_variables;
  matched_variables.insert_or_assign(base::String("--accent"),
                                     base::String("green"));
  token->SetStyleVariables(std::move(matched_variables));

  auto fragment = MakeSelectorFragmentWithToken(".matched", token);
  element->style_resolver_.CollectMatchedRules(fragment.get());

  ASSERT_EQ(StyleResolver::matched_style_map.size(), 1u);
  ASSERT_EQ(StyleResolver::matched_important_style_map.size(), 1u);
  ASSERT_EQ(StyleResolver::matched_variable_map.size(), 1u);
  ExpectPxStyle(*StyleResolver::matched_style_map[0],
                CSSPropertyID::kPropertyIDWidth, 120);
  ExpectPxStyle(*StyleResolver::matched_important_style_map[0],
                CSSPropertyID::kPropertyIDHeight, 24);

  StyleResolver::matched_style_map.clear();
  StyleResolver::matched_important_style_map.clear();
  StyleResolver::matched_variable_map.clear();
}

TEST_F(CSSPatchingTest,
       NewStylingAnalyzeMatchedResultClearsResultAndCollectsCustomProperties) {
  auto element = manager->CreateFiberView();
  StyleMap stale_result;
  stale_result.insert_or_assign(CSSPropertyID::kPropertyIDHeight,
                                CSSValue(99, CSSValuePattern::NUMBER));
  CSSVariableMap matched_variables;
  matched_variables.insert_or_assign(base::String("--accent"),
                                     base::String("blue"));
  StyleResolver::matched_variable_map.push_back(&matched_variables);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  element->style_resolver_.AnalyzeMatchedResult(style, stale_result, 0);

  EXPECT_TRUE(stale_result.empty());
  ExpectResolvedVariable(style, "--accent", "blue");
  StyleResolver::matched_variable_map.clear();
}

TEST_F(CSSPatchingTest,
       NewStylingAnalyzeMatchedResultClearsVariableDependentIds) {
  auto element = manager->CreateFiberView();
  StyleMap matched_styles;
  matched_styles.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth,
      ParseVariableValue("var(--width)", manager->GetCSSParserConfigs()));
  CSSVariableMap matched_variables;
  matched_variables.insert_or_assign(base::String("--width"),
                                     base::String("24px"));
  StyleResolver::matched_style_map.push_back(&matched_styles);
  StyleResolver::matched_variable_map.push_back(&matched_variables);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleMap result;
  CSSIDBitset variable_dependent_ids;
  variable_dependent_ids.Set(CSSPropertyID::kPropertyIDHeight);

  element->style_resolver_.AnalyzeMatchedResult(
      style, result, 0, nullptr, nullptr, &variable_dependent_ids);

  const bool still_has_stale_id =
      variable_dependent_ids.Has(CSSPropertyID::kPropertyIDHeight);
  const bool has_current_var_id =
      variable_dependent_ids.Has(CSSPropertyID::kPropertyIDWidth);
  StyleResolver::matched_style_map.clear();
  StyleResolver::matched_variable_map.clear();

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDWidth, 24);
  EXPECT_FALSE(still_has_stale_id);
  EXPECT_TRUE(has_current_var_id);
}

TEST_F(CSSPatchingTest,
       NewStylingCollectStaticStyleInputsCollectsEachInputSource) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);
  auto element = manager->CreateFiberView();

  StyleMap matched_styles;
  matched_styles.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                  CSSValue(11, CSSValuePattern::PX));
  StyleResolver::matched_style_map.push_back(&matched_styles);
  StyleMap matched_important_styles;
  matched_important_styles.insert_or_assign(CSSPropertyID::kPropertyIDHeight,
                                            CSSValue(18, CSSValuePattern::PX));
  StyleResolver::matched_important_style_map.push_back(
      &matched_important_styles);
  CSSVariableMap matched_variables;
  matched_variables.insert_or_assign(base::String("--matched"),
                                     base::String("red"));
  StyleResolver::matched_variable_map.push_back(&matched_variables);

  element->SetRawInlineStyles(
      "min-width:22px; max-width:44px !important; --inline: green;");

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleResolver::NewPipelineCollectedStyleInputs inputs;
  element->style_resolver_.CollectStaticStyleInputs(style, inputs, 0);

  ExpectPxStyle(inputs.matched_styles, CSSPropertyID::kPropertyIDWidth, 11);
  ExpectPxStyle(inputs.matched_important_styles,
                CSSPropertyID::kPropertyIDHeight, 18);
  ExpectPxStyle(inputs.inline_styles, CSSPropertyID::kPropertyIDMinWidth, 22);
  ExpectPxStyle(inputs.inline_important_styles,
                CSSPropertyID::kPropertyIDMaxWidth, 44);
  ExpectResolvedVariable(style, "--matched", "red");
  auto inline_value = style.ResolveVariable(base::String("--inline"));
  ASSERT_TRUE(inline_value.has_value());
  EXPECT_EQ(inline_value.value().AsStdString(), "green");

  StyleResolver::matched_style_map.clear();
  StyleResolver::matched_important_style_map.clear();
  StyleResolver::matched_variable_map.clear();
}

TEST_F(CSSPatchingTest,
       NewStylingSelectorExtremeParsedStylesCollectsCommittedAttributeStyles) {
  auto element = manager->CreateFiberScrollView("scroll-view");
  element->CacheStyleFromAttributes(CSSPropertyID::kPropertyIDHeight,
                                    CSSValue(33, CSSValuePattern::PX));

  StyleMap parsed_style_map;
  parsed_style_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                    CSSValue(48, CSSValuePattern::PX));
  ParsedStyles parsed_styles{std::move(parsed_style_map), CSSVariableMap{}};
  lepus::Value config = lepus::Value(lepus::Dictionary::Create());
  config.SetProperty("selectorParsedStyles", lepus::Value(true));
  element->SetParsedStyles(parsed_styles, config);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleResolver::NewPipelineCollectedStyleInputs inputs;
  element->style_resolver_.CollectStaticStyleInputs(style, inputs, 0);

  ExpectPxStyle(inputs.matched_styles, CSSPropertyID::kPropertyIDWidth, 48);
  ExpectPxStyle(inputs.attribute_styles, CSSPropertyID::kPropertyIDHeight, 33);
}

TEST_F(CSSPatchingTest,
       NewStylingCollectMatchedSpecifiedStylesResolvesLogicalProperties) {
  auto element = manager->CreateFiberView();
  StyleMap low_priority;
  low_priority.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                CSSValue(10, CSSValuePattern::PX));
  StyleMap high_priority;
  high_priority.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                 CSSValue(20, CSSValuePattern::PX));
  high_priority.insert_or_assign(CSSPropertyID::kPropertyIDMarginInlineStart,
                                 CSSValue(7, CSSValuePattern::PX));
  StyleResolver::matched_style_map.push_back(&low_priority);
  StyleResolver::matched_style_map.push_back(&high_priority);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  style.SetValue(CSSPropertyID::kPropertyIDDirection,
                 CSSValue(starlight::DirectionType::kRtl), false);
  StyleMap result;
  element->style_resolver_.CollectMatchedSpecifiedStyles(
      style, result, 0, StyleResolver::matched_style_map);

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDWidth, 20);
  ExpectNoStyle(result, CSSPropertyID::kPropertyIDMarginInlineStart);
  ExpectNoStyle(result, CSSPropertyID::kPropertyIDMarginLeft);
  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginRight, 7);
  StyleResolver::matched_style_map.clear();
}

TEST_F(CSSPatchingTest,
       NewStylingCollectInlineSpecifiedStylesParsesRawValuesAndKeepsVarTokens) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);
  auto element = manager->CreateFiberView();
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("24px"));
  element->SetStyle(CSSPropertyID::kPropertyIDMarginInlineStart,
                    lepus::Value("3px"));
  element->SetStyle(CSSPropertyID::kPropertyIDBackgroundColor,
                    lepus::Value("var(--accent)"));

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleMap result;
  element->style_resolver_.CollectInlineSpecifiedStyles(style, result, false);

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDWidth, 24);
  ExpectNoStyle(result, CSSPropertyID::kPropertyIDMarginInlineStart);
  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginLeft, 3);
  auto background = result.find(CSSPropertyID::kPropertyIDBackgroundColor);
  ASSERT_TRUE(background != result.end());
  EXPECT_TRUE(background->second.IsVariable());
}

TEST_F(CSSPatchingTest,
       NewStylingCollectMatchedCustomPropertiesParsesTlsVariables) {
  auto element = manager->CreateFiberView();
  CSSVariableMap matched_variables;
  matched_variables.insert_or_assign(base::String("--accent"),
                                     base::String("purple"));
  StyleResolver::matched_variable_map.push_back(&matched_variables);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  element->style_resolver_.CollectMatchedCustomProperties(style);

  ExpectResolvedVariable(style, "--accent", "purple");
  StyleResolver::matched_variable_map.clear();
}

TEST_F(CSSPatchingTest,
       NewStylingCollectInlineCustomPropertiesRequiresInlineVariableConfig) {
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);
  auto element = manager->CreateFiberView();
  element->SetRawInlineStyles("--accent: teal;");
  element->ProcessFullRawInlineStyle(nullptr);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  element->style_resolver_.CollectInlineCustomProperties(style);

  ExpectResolvedVariable(style, "--accent", "teal");
}

TEST_F(
    CSSPatchingTest,
    NewStylingCollectHolderCustomPropertiesAppliesHolderThenInlineVariables) {
  auto element = manager->CreateFiberView();
  CSSVariableMap holder_variables;
  holder_variables.insert_or_assign(base::String("--accent"),
                                    base::String("red"));
  holder_variables.insert_or_assign(base::String("--holder-only"),
                                    base::String("yellow"));
  element->data_model()->set_css_variables_map(std::move(holder_variables));
  element->data_model()->UpdateCSSInlineVariables(base::String("--accent"),
                                                  base::String("green"));

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  element->style_resolver_.CollectHolderCustomProperties(style);

  ExpectResolvedVariable(style, "--accent", "green");
  auto holder_only = style.ResolveVariable(base::String("--holder-only"));
  ASSERT_TRUE(holder_only.has_value());
  EXPECT_EQ(holder_only.value().AsStdString(), "yellow");
}

TEST_F(CSSPatchingTest,
       NewStylingResolveSpecifiedStyleMapUsesComputedCustomProperties) {
  auto element = manager->CreateFiberView();

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  style.SetCustomProperty(base::String("--offset"),
                          CSSValue::MakePlainString("32px"));
  style.FinalizeCustomProperties();

  CSSValue var_value("{{--offset}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  var_value.ToVarReference();
  StyleMap source;
  source.insert_or_assign(CSSPropertyID::kPropertyIDTop, var_value);

  StyleMap result;
  CSSIDBitset variable_dependent_ids;
  element->style_resolver_.ResolveSpecifiedStyleMap(style, source, result,
                                                    &variable_dependent_ids);

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDTop, 32);
  EXPECT_TRUE(variable_dependent_ids.Has(CSSPropertyID::kPropertyIDTop));
}

TEST_F(CSSPatchingTest,
       NewStylingFinalizeCustomPropertiesAppliesOverridesAndTracksVariables) {
  auto element = manager->CreateFiberView();
  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleResolver::NewPipelineCollectedStyleInputs inputs;

  CustomPropertiesMap custom_property_overrides;
  custom_property_overrides.insert_or_assign(base::String("--accent"),
                                             CSSValue::MakePlainString("blue"));

  CSSValue var_value("{{--accent}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  var_value.ToVarReference();
  StyleMap property_overrides;
  property_overrides.insert_or_assign(CSSPropertyID::kPropertyIDColor,
                                      var_value);

  element->style_resolver_.FinalizeCustomProperties(
      style, inputs, &custom_property_overrides, &property_overrides);

  ExpectResolvedVariable(style, "--accent", "blue");
  EXPECT_TRUE(manager->GetRequireCSSVariables());
}

TEST_F(CSSPatchingTest,
       NewStylingResolveCollectedStyleInputsResolvesVarsAndOverlaysOverrides) {
  auto element = manager->CreateFiberView();
  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  style.SetCustomProperty(base::String("--offset"),
                          CSSValue::MakePlainString("8px"));
  style.FinalizeCustomProperties();

  CSSValue var_value("{{--offset}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  var_value.ToVarReference();

  StyleResolver::NewPipelineCollectedStyleInputs inputs;
  inputs.matched_styles.insert_or_assign(CSSPropertyID::kPropertyIDTop,
                                         var_value);
  inputs.inline_styles.insert_or_assign(CSSPropertyID::kPropertyIDLeft,
                                        CSSValue(2, CSSValuePattern::PX));

  StyleMap property_overrides;
  property_overrides.insert_or_assign(CSSPropertyID::kPropertyIDTop,
                                      CSSValue(12, CSSValuePattern::PX));

  StyleMap result;
  element->style_resolver_.ResolveCollectedStyleInputs(style, inputs, result,
                                                       &property_overrides);

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDTop, 12);
  ExpectPxStyle(result, CSSPropertyID::kPropertyIDLeft, 2);
}

TEST_F(CSSPatchingTest,
       NewStylingApplyResolvedStyleMapReplaysInheritedSideEffects) {
  manager->config_->SetEnableCSSInheritance(true);
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  style.SetFontSize(20.0, 16.0);
  style.SetResolvedValue(CSSPropertyID::kPropertyIDFontSize,
                         CSSValue(2, CSSValuePattern::EM));

  StyleMap style_map;
  style_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                             CSSValue(24, CSSValuePattern::PX));

  element->style_resolver_.ApplyResolvedStyleMap(style, style_map);

  EXPECT_DOUBLE_EQ(style.GetFontSize(), 40.0);
  EXPECT_TRUE(style.GetChangedBitset().Has(CSSPropertyID::kPropertyIDFontSize));
  ExpectNumberStyle(style.GetResolvedValues(),
                    CSSPropertyID::kPropertyIDFontSize, 40);
  ExpectPxStyle(style.GetResolvedValues(), CSSPropertyID::kPropertyIDWidth, 24);
}

TEST_F(CSSPatchingTest,
       NewStylingApplyResolvedStyleMapEmptyInputReplaysInheritedSideEffects) {
  manager->config_->SetEnableCSSInheritance(true);
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberText("text");
  page->InsertNode(element);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  style.SetFontSize(20.0, 16.0);
  style.SetValue(CSSPropertyID::kPropertyIDDirection,
                 CSSValue(starlight::DirectionType::kRtl));
  style.SetValue(CSSPropertyID::kPropertyIDTextAlign,
                 CSSValue(starlight::TextAlignType::kStart));
  style.SetResolvedValue(CSSPropertyID::kPropertyIDFontSize,
                         CSSValue(2, CSSValuePattern::EM));

  StyleMap style_map;
  element->style_resolver_.ApplyResolvedStyleMap(style, style_map);

  EXPECT_DOUBLE_EQ(style.GetFontSize(), 40.0);
  EXPECT_TRUE(style.GetChangedBitset().Has(CSSPropertyID::kPropertyIDFontSize));
  ExpectNumberStyle(style.GetResolvedValues(),
                    CSSPropertyID::kPropertyIDFontSize, 40);
  ASSERT_TRUE(style.GetTextAttributes().has_value());
  EXPECT_EQ(style.GetTextAttributes()->text_align,
            starlight::TextAlignType::kRight);
}

TEST_F(CSSPatchingTest,
       NewStylingApplyResolvedStyleMapDoesNotMaterializeDefaultInputDirty) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleMap style_map;
  style_map.insert_or_assign(CSSPropertyID::kPropertyIDOpacity,
                             CSSValue(1.0, CSSValuePattern::NUMBER));

  element->style_resolver_.ApplyResolvedStyleMap(style, style_map);

  EXPECT_FALSE(style.GetChangedBitset().Has(CSSPropertyID::kPropertyIDOpacity));
  ExpectNumberStyle(style.GetResolvedValues(),
                    CSSPropertyID::kPropertyIDOpacity, 1.0);
}

TEST_F(CSSPatchingTest,
       NewStylingApplyResolvedStyleMapReanalyzesAfterDirectionChange) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->data_model()->set_tag("view");
  element->data_model()->SetClass("matched");
  element->SetStyle(CSSPropertyID::kPropertyIDDirection, lepus::Value("rtl"));

  CSSParserConfigs configs;
  auto tokens = fml::MakeRefCounted<CSSParseToken>(configs);
  tokens->raw_attributes_[CSSPropertyID::kPropertyIDMarginInlineStart] =
      CSSValue::MakePlainString("6px");
  std::string key = ".matched";
  tokens->sheets().emplace_back(std::make_shared<CSSSheet>(key));
  CSSParserTokenMap css_map;
  css_map.insert_or_assign(key, tokens);
  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment fragment(1, dependent_ids, css_map, keyframes, fontfaces);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  element->style_resolver_.CollectMatchedRules(&fragment);
  StyleMap result;
  element->style_resolver_.AnalyzeMatchedResult(style, result, 0);

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginLeft, 6);

  element->style_resolver_.ApplyResolvedStyleMap(style, result);

  EXPECT_EQ(style.GetDirection(), starlight::DirectionType::kRtl);
  ExpectNoStyle(result, CSSPropertyID::kPropertyIDMarginLeft);
  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginRight, 6);
  EXPECT_TRUE(StyleResolver::matched_style_map.empty());
  EXPECT_TRUE(StyleResolver::matched_important_style_map.empty());
  EXPECT_TRUE(StyleResolver::matched_variable_map.empty());
}

TEST_F(CSSPatchingTest,
       NewStylingApplyResolvedStyleMapReanalyzesInlineAfterDirectionChange) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDDirection, lepus::Value("rtl"));
  element->SetStyle(CSSPropertyID::kPropertyIDMarginInlineStart,
                    lepus::Value("6px"));

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleMap result;
  element->style_resolver_.AnalyzeMatchedResult(style, result, 0);

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginLeft, 6);

  element->style_resolver_.ApplyResolvedStyleMap(style, result);

  EXPECT_EQ(style.GetDirection(), starlight::DirectionType::kRtl);
  ExpectNoStyle(result, CSSPropertyID::kPropertyIDMarginLeft);
  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginRight, 6);
}

TEST_F(
    CSSPatchingTest,
    NewStylingApplyResolvedStyleMapReanalyzesImportantInlineAfterDirectionChange) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->SetRawInlineStyles(
      "direction: rtl !important; margin-inline-start: 6px !important;");

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleMap result;
  element->style_resolver_.AnalyzeMatchedResult(style, result, 0);

  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginLeft, 6);

  element->style_resolver_.ApplyResolvedStyleMap(style, result);

  EXPECT_EQ(style.GetDirection(), starlight::DirectionType::kRtl);
  ExpectNoStyle(result, CSSPropertyID::kPropertyIDMarginLeft);
  ExpectPxStyle(result, CSSPropertyID::kPropertyIDMarginRight, 6);
}

TEST_F(CSSPatchingTest, NewStylingApplyResolvedStyleMapNormalizesTextAlign) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberText("text");
  page->InsertNode(element);

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleMap style_map;
  style_map.insert_or_assign(CSSPropertyID::kPropertyIDDirection,
                             CSSValue(starlight::DirectionType::kRtl));
  style_map.insert_or_assign(CSSPropertyID::kPropertyIDTextAlign,
                             CSSValue(starlight::TextAlignType::kStart));

  element->style_resolver_.ApplyResolvedStyleMap(style, style_map);

  ASSERT_TRUE(style.GetTextAttributes().has_value());
  EXPECT_EQ(style.GetTextAttributes()->text_align,
            starlight::TextAlignType::kRight);
}

TEST_F(CSSPatchingTest, NewStylingResolveBaseStyleInPlaceAppliesInlineStyle) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("40px"));

  starlight::ComputedCSSStyle style(*manager->platform_computed_css());
  StyleMap resolved_style_map;
  element->style_resolver_.ResolveBaseStyleInPlace(style, nullptr, nullptr,
                                                   &resolved_style_map);

  ExpectPxStyle(resolved_style_map, CSSPropertyID::kPropertyIDWidth, 40);
  ExpectPxStyle(style.GetResolvedValues(), CSSPropertyID::kPropertyIDWidth, 40);
}

TEST_F(CSSPatchingTest, NewStylingResolveBaseStyleReturnsResolvedStyle) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("40px"));

  StyleMap resolved_style_map;
  auto style = element->style_resolver_.ResolveBaseStyle(nullptr, nullptr,
                                                         &resolved_style_map);

  ASSERT_NE(style, nullptr);
  ExpectPxStyle(resolved_style_map, CSSPropertyID::kPropertyIDWidth, 40);
  ExpectPxStyle(style->GetResolvedValues(), CSSPropertyID::kPropertyIDWidth,
                40);
}

TEST_F(CSSPatchingTest,
       NewStylingResolveBaseStyleAppliesTextDecorationExtensionValues) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberText("text");
  page->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDTextDecorationThickness,
                    lepus::Value("2px"));
  element->SetStyle(CSSPropertyID::kPropertyIDXTextDecorationWidth,
                    lepus::Value("8px"));
  element->SetStyle(CSSPropertyID::kPropertyIDXTextDecorationGap,
                    lepus::Value("4px"));

  StyleMap resolved_style_map;
  auto style = element->style_resolver_.ResolveBaseStyle(nullptr, nullptr,
                                                         &resolved_style_map);

  ASSERT_NE(style, nullptr);
  ExpectPxStyle(resolved_style_map,
                CSSPropertyID::kPropertyIDTextDecorationThickness, 2);
  ExpectPxStyle(resolved_style_map,
                CSSPropertyID::kPropertyIDXTextDecorationWidth, 8);
  ExpectPxStyle(resolved_style_map,
                CSSPropertyID::kPropertyIDXTextDecorationGap, 4);
  const auto& text_attributes = style->GetTextAttributes();
  ASSERT_TRUE(text_attributes.has_value());
  ASSERT_TRUE(text_attributes->text_decoration_thickness.has_value());
  EXPECT_FLOAT_EQ(*text_attributes->text_decoration_thickness, 2.f);
  ASSERT_TRUE(text_attributes->text_decoration_width.has_value());
  EXPECT_FLOAT_EQ(*text_attributes->text_decoration_width, 8.f);
  ASSERT_TRUE(text_attributes->text_decoration_gap.has_value());
  EXPECT_FLOAT_EQ(*text_attributes->text_decoration_gap, 4.f);
  EXPECT_TRUE(
      HasChanged(*style, CSSPropertyID::kPropertyIDTextDecorationThickness));
  EXPECT_TRUE(
      HasChanged(*style, CSSPropertyID::kPropertyIDXTextDecorationWidth));
  EXPECT_TRUE(HasChanged(*style, CSSPropertyID::kPropertyIDXTextDecorationGap));
}

TEST_F(CSSPatchingTest,
       NewStylingResolveBaseStyleConsumesCommittedAttributeStyles) {
  auto page = CreatePageRoot(16.0);
  auto scroll = manager->CreateFiberScrollView("scroll-view");
  page->InsertNode(scroll);
  scroll->SetAttribute("scroll-x", lepus::Value("true"));
  page->FlushActionsAsRoot();

  StyleMap resolved_style_map;
  auto style = scroll->style_resolver_.ResolveBaseStyle(nullptr, nullptr,
                                                        &resolved_style_map);

  ASSERT_NE(style, nullptr);
  auto it =
      resolved_style_map.find(CSSPropertyID::kPropertyIDLinearOrientation);
  ASSERT_TRUE(it != resolved_style_map.end());
  EXPECT_EQ(it->second,
            CSSValue(starlight::LinearOrientationType::kHorizontal));
  auto resolved_it = style->GetResolvedValues().find(
      CSSPropertyID::kPropertyIDLinearOrientation);
  ASSERT_TRUE(resolved_it != style->GetResolvedValues().end());
  EXPECT_EQ(resolved_it->second,
            CSSValue(starlight::LinearOrientationType::kHorizontal));
}

TEST_F(CSSPatchingTest,
       NewStylingResolvePseudoElementsUsesRegisteredPredicates) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->has_placeholder_ = true;
  element->has_text_selection_ = true;

  SharedCSSFragment fragment;
  CSSParserConfigs configs;
  auto placeholder_token = fml::MakeRefCounted<CSSParseToken>(configs);
  placeholder_token->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("blue");
  fragment.pseudo_map_.emplace(kCSSSelectorPlaceholder, placeholder_token);

  auto selection_token = fml::MakeRefCounted<CSSParseToken>(configs);
  selection_token->raw_attributes_[CSSPropertyID::kPropertyIDBackgroundColor] =
      CSSValue::MakePlainString("red");
  fragment.pseudo_map_.emplace(kCSSSelectorSelection, selection_token);

  element->style_resolver_.ResolvePseudoElementsForNewPipeline(&fragment);

  ASSERT_TRUE(element->pseudo_elements_.has_value());
  auto placeholder_it =
      element->pseudo_elements_->find(kPseudoStatePlaceHolder);
  ASSERT_TRUE(placeholder_it != element->pseudo_elements_->end());
  EXPECT_TRUE(placeholder_it->second->style_map_.contains(kPropertyIDColor));

  auto selection_it = element->pseudo_elements_->find(kPseudoStateSelection);
  ASSERT_TRUE(selection_it != element->pseudo_elements_->end());
  EXPECT_TRUE(
      selection_it->second->style_map_.contains(kPropertyIDBackgroundColor));
}

TEST_F(CSSPatchingTest,
       NewStylingResolvePseudoElementsUsesAdoptedStylesheetPseudoRules) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->has_placeholder_ = true;

  MockCSSFragment base_fragment;
  base_fragment.SetEnableCSSSelector(true);

  auto adopted_fragment = std::make_unique<MockCSSFragment>();
  adopted_fragment->SetEnableCSSSelector(true);

  CSSParserConfigs configs;
  auto placeholder_token = fml::MakeRefCounted<CSSParseToken>(configs);
  placeholder_token->raw_attributes_[CSSPropertyID::kPropertyIDColor] =
      CSSValue::MakePlainString("blue");

  auto placeholder_selector = std::make_unique<LynxCSSSelector[]>(1);
  placeholder_selector[0].SetMatch(LynxCSSSelector::kPseudoElement);
  placeholder_selector[0].SetPseudoType(LynxCSSSelector::kPseudoPlaceholder);
  placeholder_selector[0].SetValue("placeholder");
  placeholder_selector[0].SetLastInTagHistory(true);
  placeholder_selector[0].SetLastInSelectorList(true);
  adopted_fragment->AddStyleRule(std::move(placeholder_selector),
                                 placeholder_token);

  ASSERT_TRUE(base_fragment.rule_set()->pseudo_rules().empty());
  ASSERT_FALSE(adopted_fragment->rule_set()->pseudo_rules().empty());

  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  wrapper->fragment_ = std::move(adopted_fragment);
  manager->AdoptStyleSheet(wrapper);

  CSSFragmentDecorator decorator(&base_fragment, manager.get());
  element->style_resolver_.ResolvePseudoElementsForNewPipeline(&decorator);

  ASSERT_TRUE(element->pseudo_elements_.has_value());
  auto placeholder_it =
      element->pseudo_elements_->find(kPseudoStatePlaceHolder);
  ASSERT_TRUE(placeholder_it != element->pseudo_elements_->end());
  EXPECT_TRUE(placeholder_it->second->style_map_.contains(kPropertyIDColor));
}

TEST_F(CSSPatchingTest,
       NewStylingLegacyAnimatorDetectsAnimationAndTransitionDataChanges) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->ResetAllDirtyBits();

  starlight::ComputedCSSStyle old_style(*manager->platform_computed_css());
  old_style.animation_data().emplace_back();
  old_style.animation_data()[0].name = base::String("move");
  old_style.animation_data()[0].duration = 2000;
  old_style.transition_data().durations.emplace_back(1000);
  old_style.ClearChanged();
  old_style.ClearReset();

  starlight::ComputedCSSStyle new_style(*manager->platform_computed_css());
  new_style.CopyFrom(old_style);
  new_style.animation_data().clear();
  new_style.animation_data().emplace_back();
  new_style.animation_data()[0].name = base::String("move");
  new_style.animation_data()[0].duration = 2300;
  new_style.animation_data().emplace_back();
  new_style.animation_data()[1].name = base::String("color");
  new_style.animation_data()[1].duration = 1000;
  new_style.animation_data().emplace_back();
  new_style.animation_data()[2].name = base::String("opacity");
  new_style.animation_data()[2].duration = 700;
  new_style.transition_data().durations.clear();
  new_style.transition_data().durations.emplace_back(1300);
  new_style.ClearChanged();
  new_style.ClearReset();

  EXPECT_FALSE(new_style.IsDirty());
  EXPECT_FALSE(HasChanged(new_style, CSSPropertyID::kPropertyIDAnimation));
  EXPECT_FALSE(HasChanged(new_style, CSSPropertyID::kPropertyIDTransition));

  auto analysis = element->AnalyzeAnimationPropChangesForLegacyAnimator(
      new_style, &old_style, {});
  EXPECT_TRUE(analysis.has_keyframe_props_changed);
  EXPECT_TRUE(analysis.has_transition_props_changed);
}

TEST_F(CSSPatchingTest,
       NewStylingNewAnimatorDoesNotWriteAnimationOrTransitionToPropBundle) {
  manager->enable_new_styling_pipeline_ = true;
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  element->enable_new_animator_ = true;
  page->InsertNode(element);

  element->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                    lepus::Value("move 2000ms linear 0ms 1 normal both"));
  element->SetStyle(CSSPropertyID::kPropertyIDTransition,
                    lepus::Value("opacity 1000ms linear 0ms"));
  element->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value("1"));
  page->FlushActionsAsRoot();

  element->SetStyle(CSSPropertyID::kPropertyIDAnimation,
                    lepus::Value("move 2300ms linear 0ms 1 normal both, "
                                 "color 1000ms linear 0ms 1 normal both, "
                                 "opacity 700ms linear 0ms 1 normal both"));
  element->SetStyle(CSSPropertyID::kPropertyIDTransition,
                    lepus::Value("opacity 1300ms linear 0ms"));
  element->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value("2"));
  page->FlushActionsAsRoot();

  auto* painting_context =
      static_cast<MockPaintingContext*>(manager->painting_context()->impl());
  auto painting_node_iter =
      painting_context->node_map_.find(element->impl_id());
  ASSERT_NE(painting_node_iter, painting_context->node_map_.end());
  const auto& props = painting_node_iter->second->props_;
  auto z_index_it = props.find(
      CSSProperty::GetPropertyNameCStr(CSSPropertyID::kPropertyIDZIndex));
  ASSERT_NE(z_index_it, props.end());
  EXPECT_EQ(z_index_it->second.Number(), 2);
  EXPECT_EQ(props.find(CSSProperty::GetPropertyNameCStr(
                CSSPropertyID::kPropertyIDAnimation)),
            props.end());
  EXPECT_EQ(props.find(CSSProperty::GetPropertyNameCStr(
                CSSPropertyID::kPropertyIDTransition)),
            props.end());
}

TEST_F(CSSPatchingTest,
       NewStylingBuildFinalStyleFromBaseFastPathAppliesSampledOverrides) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle base_style(*manager->platform_computed_css());
  base_style.SetFontSize(16.0, 16.0);
  base_style.SetResolvedValue(CSSPropertyID::kPropertyIDWidth,
                              CSSValue(10, CSSValuePattern::PX));

  StyleMap sampled_property_overrides;
  sampled_property_overrides.insert_or_assign(
      CSSPropertyID::kPropertyIDFontSize, CSSValue(2, CSSValuePattern::EM));
  sampled_property_overrides.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth, CSSValue(44, CSSValuePattern::PX));

  StyleMap resolved_style_map;
  auto final_style = element->style_resolver_.BuildFinalStyleFromBaseFastPath(
      base_style, &sampled_property_overrides, &resolved_style_map);

  ASSERT_NE(final_style, nullptr);
  EXPECT_DOUBLE_EQ(final_style->GetFontSize(), 32.0);
  ExpectPxStyle(final_style->GetResolvedValues(),
                CSSPropertyID::kPropertyIDWidth, 44);
  ExpectPxStyle(resolved_style_map, CSSPropertyID::kPropertyIDWidth, 44);
}

TEST_F(
    CSSPatchingTest,
    NewStylingBuildFinalStyleFromBaseFastPathTracksVariableDependenciesWithoutResolvedMap) {
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);

  starlight::ComputedCSSStyle base_style(*manager->platform_computed_css());
  base_style.SetFontSize(16.0, 16.0);

  StyleMap sampled_property_overrides;
  sampled_property_overrides.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth,
      ParseVariableValue("var(--width)", manager->GetCSSParserConfigs()));

  CSSIDBitset variable_dependent_ids;
  auto final_style = element->style_resolver_.BuildFinalStyleFromBaseFastPath(
      base_style, &sampled_property_overrides, nullptr,
      &variable_dependent_ids);

  ASSERT_NE(final_style, nullptr);
  EXPECT_TRUE(variable_dependent_ids.Has(CSSPropertyID::kPropertyIDWidth));
}

TEST_F(CSSPatchingTest,
       NewStylingRebuildFinalStyleFromParentAppliesSampledOverrides) {
  manager->config_->SetEnableCSSInheritance(true);
  auto page = CreatePageRoot(16.0);
  auto element = manager->CreateFiberView();
  page->InsertNode(element);
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, lepus::Value("12px"));

  starlight::ComputedCSSStyle parent_style(*manager->platform_computed_css());
  parent_style.SetFontSize(20.0, 16.0);

  StyleMap sampled_property_overrides;
  sampled_property_overrides.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth, CSSValue(48, CSSValuePattern::PX));

  StyleMap resolved_style_map;
  auto final_style = element->style_resolver_.RebuildFinalStyleFromParent(
      &parent_style, nullptr, nullptr, &sampled_property_overrides,
      &resolved_style_map);

  ASSERT_NE(final_style, nullptr);
  EXPECT_DOUBLE_EQ(final_style->GetFontSize(), 20.0);
  ExpectPxStyle(resolved_style_map, CSSPropertyID::kPropertyIDWidth, 48);
  ExpectPxStyle(final_style->GetResolvedValues(),
                CSSPropertyID::kPropertyIDWidth, 48);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
