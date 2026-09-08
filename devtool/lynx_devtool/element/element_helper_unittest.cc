// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/element/element_helper.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

#include "base/include/auto_reset.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/parser/media_query_parser.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/ng/style/condition_rule.h"
#include "core/renderer/css/ng/style/style_rule.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "devtool/lynx_devtool/element/helper_util.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

lynx::tasm::CSSValue ParseVariableValue(const char* raw_value) {
  lynx::tasm::CSSParserConfigs configs;
  lynx::lepus::Value value(raw_value);
  auto parser = lynx::tasm::CSSStringParser::FromLepusString(value, configs);
  return parser.ParseVariable();
}

std::unique_ptr<lynx::css::LynxCSSSelector[]> CreateSelectorArray(
    const std::string& selector) {
  lynx::css::CSSParserContext context;
  lynx::css::CSSTokenizer tokenizer(selector);
  auto parser_tokens = tokenizer.TokenizeToEOF();
  lynx::css::CSSParserTokenRange range(parser_tokens);
  auto selector_vector =
      lynx::css::CSSSelectorParser::ParseSelector(range, &context);
  size_t flattened_size =
      lynx::css::CSSSelectorParser::FlattenedSize(selector_vector);
  auto selector_arr =
      std::make_unique<lynx::css::LynxCSSSelector[]>(flattened_size);
  lynx::css::CSSSelectorParser::AdoptSelectorVector(
      selector_vector, selector_arr.get(), flattened_size);
  return selector_arr;
}

fml::RefPtr<lynx::tasm::CSSParseToken> CreateParseToken(
    lynx::tasm::CSSPropertyID property_id, const std::string& value,
    bool important = false) {
  lynx::tasm::CSSParserConfigs parser_configs;
  auto token = fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  auto& attributes =
      important ? token->raw_important_attributes_ : token->raw_attributes_;
  attributes[property_id] =
      lynx::tasm::CSSValue::MakePlainString(value.c_str());
  return token;
}

std::shared_ptr<lynx::tasm::SharedCSSFragment> CreateSelectorCSSFragment(
    const std::string& selector, lynx::tasm::CSSPropertyID property_id,
    const std::string& value) {
  auto token = CreateParseToken(property_id, value);

  auto sheet = std::make_shared<lynx::tasm::CSSSheet>(selector);
  token->sheets().emplace_back(sheet);

  lynx::tasm::CSSParserTokenMap token_map;
  token_map.insert(std::make_pair(selector, token));

  const std::vector<int32_t> dependent_ids;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<lynx::tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();

  fragment->AddStyleRule(CreateSelectorArray(selector), token);

  return fragment;
}

std::shared_ptr<lynx::tasm::SharedCSSFragment> CreateLayeredCSSFragment(
    const std::string& selector, lynx::tasm::CSSPropertyID property_id,
    const std::string& value, const std::vector<std::string>& layer_path,
    bool important = false) {
  auto token = CreateParseToken(property_id, value, important);

  auto sheet = std::make_shared<lynx::tasm::CSSSheet>(selector);
  token->sheets().emplace_back(sheet);

  lynx::tasm::CSSParserTokenMap token_map;
  token_map.insert(std::make_pair(selector, token));

  const std::vector<int32_t> dependent_ids;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<lynx::tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();
  fragment->SetEnableCSSRule(true);

  auto* layer = fragment->GetOrCreateRootLayer()->GetOrAddSubLayer(layer_path);
  fragment->AddStyleRule(fml::MakeRefCounted<lynx::css::StyleRule>(
                             CreateSelectorArray(selector), token),
                         layer);
  return fragment;
}

struct LayeredCascadeRuleForTest {
  std::string value;
  std::vector<std::string> layer_path;
  bool important;
};

std::shared_ptr<lynx::tasm::SharedCSSFragment> CreateLayeredCascadeCSSFragment(
    const std::string& selector, lynx::tasm::CSSPropertyID property_id,
    std::initializer_list<LayeredCascadeRuleForTest> rules) {
  std::vector<fml::RefPtr<lynx::tasm::CSSParseToken>> tokens;
  tokens.reserve(rules.size());
  lynx::tasm::CSSParserTokenMap token_map;
  for (const auto& rule : rules) {
    auto token = CreateParseToken(property_id, rule.value, rule.important);
    auto sheet = std::make_shared<lynx::tasm::CSSSheet>(selector);
    token->sheets().emplace_back(std::move(sheet));
    if (token_map.empty()) {
      token_map.insert({selector, token});
    }
    tokens.push_back(std::move(token));
  }

  const std::vector<int32_t> dependent_ids;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<lynx::tasm::SharedCSSFragment>(
      1, dependent_ids, std::move(token_map), std::move(keyframes),
      std::move(font_faces));
  fragment->SetEnableCSSSelector();
  fragment->SetEnableCSSRule(true);

  size_t index = 0;
  for (const auto& rule : rules) {
    lynx::css::CascadeLayer* layer = nullptr;
    if (!rule.layer_path.empty()) {
      layer =
          fragment->GetOrCreateRootLayer()->GetOrAddSubLayer(rule.layer_path);
    }
    fragment->AddStyleRule(fml::MakeRefCounted<lynx::css::StyleRule>(
                               CreateSelectorArray(selector), tokens[index++]),
                           layer);
  }
  return fragment;
}

std::shared_ptr<lynx::tasm::SharedCSSFragment>
CreateSplitTokenSelectorCSSFragment(
    const std::string& selector, lynx::tasm::CSSPropertyID property_id,
    const std::string& value,
    fml::RefPtr<lynx::tasm::CSSParseToken>* shared_token,
    fml::RefPtr<lynx::tasm::CSSParseToken>* matched_token) {
  *shared_token = CreateParseToken(property_id, value);
  *matched_token = CreateParseToken(property_id, value);

  auto sheet = std::make_shared<lynx::tasm::CSSSheet>(selector);
  (*matched_token)->sheets().emplace_back(sheet);

  lynx::tasm::CSSParserTokenMap token_map;
  token_map.insert(std::make_pair(selector, *shared_token));

  const std::vector<int32_t> dependent_ids;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<lynx::tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();
  fragment->AddStyleRule(CreateSelectorArray(selector), *matched_token);
  return fragment;
}

std::shared_ptr<lynx::tasm::SharedCSSFragment> CreateCustomPropertyCSSFragment(
    const std::string& selector, const std::string& name,
    const std::string& value) {
  lynx::tasm::CSSParserConfigs parser_configs;
  auto token = fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  lynx::tasm::CSSVariableMap variables;
  variables.insert_or_assign(lynx::base::String(name),
                             lynx::base::String(value));
  token->SetStyleVariables(std::move(variables));
  token->MarkParsed();

  auto sheet = std::make_shared<lynx::tasm::CSSSheet>(selector);
  token->sheets().emplace_back(sheet);

  lynx::tasm::CSSParserTokenMap token_map;
  token_map.insert(std::make_pair(selector, token));

  const std::vector<int32_t> dependent_ids;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<lynx::tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();
  fragment->AddStyleRule(CreateSelectorArray(selector), token);
  return fragment;
}

std::shared_ptr<lynx::tasm::SharedCSSFragment>
CreateSplitTokenCustomPropertyCSSFragment(
    const std::string& selector, const std::string& name,
    const std::string& value,
    fml::RefPtr<lynx::tasm::CSSParseToken>* shared_token,
    fml::RefPtr<lynx::tasm::CSSParseToken>* matched_token) {
  lynx::tasm::CSSParserConfigs parser_configs;
  *shared_token =
      fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  *matched_token =
      fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  for (const auto& token : {*shared_token, *matched_token}) {
    lynx::tasm::CSSVariableMap variables;
    variables.insert_or_assign(lynx::base::String(name),
                               lynx::base::String(value));
    token->SetStyleVariables(std::move(variables));
    token->MarkParsed();
  }

  auto sheet = std::make_shared<lynx::tasm::CSSSheet>(selector);
  (*matched_token)->sheets().emplace_back(sheet);

  lynx::tasm::CSSParserTokenMap token_map;
  token_map.insert(std::make_pair(selector, *shared_token));
  const std::vector<int32_t> dependent_ids;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<lynx::tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();
  fragment->AddStyleRule(CreateSelectorArray(selector), *matched_token);
  return fragment;
}

struct MediaRuleForTest {
  std::string selector;
  lynx::tasm::CSSPropertyID property_id;
  std::string value;
  std::string media_text;
  std::vector<std::string> layer_path;
};

std::shared_ptr<lynx::tasm::SharedCSSFragment> CreateMediaCSSFragment(
    std::initializer_list<MediaRuleForTest> rules) {
  lynx::tasm::CSSParserTokenMap token_map;
  std::vector<fml::RefPtr<lynx::tasm::CSSParseToken>> tokens;
  tokens.reserve(rules.size());

  for (const auto& rule : rules) {
    auto token = CreateParseToken(rule.property_id, rule.value);
    auto sheet = std::make_shared<lynx::tasm::CSSSheet>(rule.selector);
    token->sheets().emplace_back(sheet);
    token_map.insert(std::make_pair(rule.selector, token));
    tokens.push_back(std::move(token));
  }

  const std::vector<int32_t> dependent_ids;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto fragment = std::make_shared<lynx::tasm::SharedCSSFragment>(
      1, dependent_ids, token_map, keyframes, font_faces);
  fragment->SetEnableCSSSelector();

  size_t index = 0;
  for (const auto& rule : rules) {
    lynx::css::CascadeLayer* layer = nullptr;
    if (!rule.layer_path.empty()) {
      fragment->SetEnableCSSRule(true);
      layer =
          fragment->GetOrCreateRootLayer()->GetOrAddSubLayer(rule.layer_path);
    }
    auto condition_rule =
        fml::MakeRefCounted<lynx::css::ConditionRule>(fragment.get());
    condition_rule->SetMediaQueries(
        lynx::css::MediaQueryParser::ParseMediaQuerySet(rule.media_text));
    condition_rule->AddStyleRule(
        fml::MakeRefCounted<lynx::css::StyleRule>(
            CreateSelectorArray(rule.selector), tokens[index++]),
        layer);
    fragment->AddConditionRule(std::move(condition_rule));
  }

  return fragment;
}

class LifetimeObservedCSSParseToken : public lynx::tasm::CSSParseToken {
 public:
  LifetimeObservedCSSParseToken(
      const lynx::tasm::CSSParserConfigs& parser_configs, bool* destroyed)
      : CSSParseToken(parser_configs), destroyed_(destroyed) {}

  ~LifetimeObservedCSSParseToken() override { *destroyed_ = true; }

 private:
  bool* destroyed_;
};

class ElementHelperTest : public ::testing::Test {
 public:
  ElementHelperTest() {}
  ~ElementHelperTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>
      tasm_mediator;

  void SetUp() override {
    lynx::tasm::LynxEnvConfig lynx_env_config(
        kWidth, kHeight, kDefaultLayoutsUnitPerPx,
        kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<lynx::tasm::MockPaintingContext>(),
        tasm_mediator.get(), lynx_env_config);
    auto config = std::make_shared<lynx::tasm::PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
};

TEST_F(ElementHelperTest, StyleTextParserTest) {
  std::unordered_multimap<std::string, lynx::devtool::CSSPropertyDetail>
      css_properties = {{"display-string",
                         {"display",
                          "linear",
                          "display:linear;",
                          false,
                          false,
                          false,
                          false,
                          true,
                          {27, 10, 12, 10}}}};
  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.origin_ = "regular";
  style_sheet.css_text_ = ".label_text";
  style_sheet.css_properties_ = css_properties;
  style_sheet.style_value_range_ = {81, 10, 12, 10};
  style_sheet.media_text_ = "(min-width: 1000px)";
  style_sheet.media_range_ = {2, 2, 0, 19};

  auto element = manager->CreateFiberElement("view");

  EXPECT_EQ(lynx::devtool::StyleTextParser(
                element.get(), "width:10px;height:10px", style_sheet)
                .origin_,
            "regular");
  EXPECT_EQ(lynx::devtool::StyleTextParser(
                element.get(), "width:10px;height:10px", style_sheet)
                .css_text_,
            "width:10px;height:10px;");

  std::unordered_multimap<std::string, lynx::devtool::CSSPropertyDetail>
      new_css_properties =
          lynx::devtool::StyleTextParser(element.get(),
                                         "width:10px;height:10px", style_sheet)
              .css_properties_;
  for (auto& it : new_css_properties) {
    if (it.first == "width") {
      EXPECT_EQ(it.second.text_, "width:10px;");
    } else if (it.first == "height") {
      EXPECT_EQ(it.second.text_, "height:10px;");
    }
  }
  auto parsed = lynx::devtool::StyleTextParser(
      element.get(), "width:10px;height:10px", style_sheet);
  EXPECT_EQ(parsed.media_text_, "(min-width: 1000px)");
  EXPECT_EQ(parsed.media_range_.start_line_, 2);
  EXPECT_EQ(parsed.media_range_.end_line_, 2);
  EXPECT_EQ(parsed.media_range_.start_column_, 0);
  EXPECT_EQ(parsed.media_range_.end_column_, 19);

  auto parsed_custom_properties =
      lynx::devtool::StyleTextParser(element.get(),
                                     "--theme:red;/* --disabled-theme:blue; */"
                                     "--animation-token:ease;",
                                     style_sheet);
  auto theme = parsed_custom_properties.css_properties_.find("--theme");
  ASSERT_NE(theme, parsed_custom_properties.css_properties_.end());
  EXPECT_TRUE(theme->second.parsed_ok_);
  EXPECT_FALSE(theme->second.disabled_);

  auto disabled_theme =
      parsed_custom_properties.css_properties_.find("--disabled-theme");
  ASSERT_NE(disabled_theme, parsed_custom_properties.css_properties_.end());
  EXPECT_TRUE(disabled_theme->second.parsed_ok_);
  EXPECT_TRUE(disabled_theme->second.disabled_);

  auto animation_token =
      parsed_custom_properties.css_properties_.find("--animation-token");
  ASSERT_NE(animation_token, parsed_custom_properties.css_properties_.end());
  EXPECT_TRUE(animation_token->second.parsed_ok_);

  auto parsed_variable_properties = lynx::devtool::StyleTextParser(
      element.get(),
      "color:var(--text-color);"
      "/* background-color:var(--background-color); */"
      "unknown-property:var(--unknown);",
      style_sheet);
  auto color = parsed_variable_properties.css_properties_.find("color");
  ASSERT_NE(color, parsed_variable_properties.css_properties_.end());
  EXPECT_TRUE(color->second.parsed_ok_);
  EXPECT_FALSE(color->second.disabled_);

  auto disabled_background =
      parsed_variable_properties.css_properties_.find("background-color");
  ASSERT_NE(disabled_background,
            parsed_variable_properties.css_properties_.end());
  EXPECT_TRUE(disabled_background->second.parsed_ok_);
  EXPECT_TRUE(disabled_background->second.disabled_);

  auto unknown_property =
      parsed_variable_properties.css_properties_.find("unknown-property");
  ASSERT_NE(unknown_property, parsed_variable_properties.css_properties_.end());
  EXPECT_FALSE(unknown_property->second.parsed_ok_);
}

TEST_F(ElementHelperTest, GetDocumentBodyFromNodeTest) {
  auto element1 = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element1));

  auto document_body1 =
      lynx::devtool::ElementHelper::GetDocumentBodyFromNode(element1.get());
  Json::Value res1 = Json::Value(Json::ValueType::objectValue);
  res1["attributes"] = Json::Value(Json::ValueType::arrayValue);
  res1["backendNodeId"] = 10;
  res1["childNodeCount"] = 0;
  res1["children"] = Json::Value(Json::ValueType::arrayValue);
  res1["localName"] = "view";
  res1["nodeId"] = 10;
  res1["nodeName"] = "VIEW";
  res1["nodeType"] = 1;
  res1["nodeValue"] = "";
  EXPECT_EQ(document_body1, res1);

  auto element = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  auto element_container = element->element_container_impl();

  auto child = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  element->AddChildAt(child, 0);
  EXPECT_EQ(child->parent(), element.get());

  child->CreateElementContainer(false);
  auto child_container = child->element_container_impl();
  child_container->InsertSelf();
  EXPECT_EQ(child_container->parent(), element_container);
  EXPECT_EQ(element_container->children().size(), static_cast<size_t>(1));

  auto document_body =
      lynx::devtool::ElementHelper::GetDocumentBodyFromNode(element.get());
  Json::Value res = Json::Value(Json::ValueType::objectValue);
  res["attributes"] = Json::Value(Json::ValueType::arrayValue);
  res["backendNodeId"] = 11;
  res["childNodeCount"] = 1;
  Json::Value child0 = Json::Value(Json::ValueType::objectValue);
  child0["attributes"] = Json::Value(Json::ValueType::arrayValue);
  child0["backendNodeId"] = 12;
  child0["childNodeCount"] = 0;
  child0["children"] = Json::Value(Json::ValueType::arrayValue);
  child0["localName"] = "view";
  child0["nodeId"] = 12;
  child0["nodeName"] = "VIEW";
  child0["nodeType"] = 1;
  child0["nodeValue"] = "";
  child0["parentId"] = 11;
  res["children"].append(child0);
  res["localName"] = "view";
  res["nodeId"] = 11;
  res["nodeName"] = "VIEW";
  res["nodeType"] = 1;
  res["nodeValue"] = "";
  EXPECT_EQ(document_body, res);
  child_container->RemoveSelf(true);
}

TEST_F(ElementHelperTest, GetDocumentBodyFromNodeWithDepthTest) {
  auto root = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(root.get()));
  root->CreateElementContainer(false);

  auto child = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  root->AddChildAt(child, 0);
  child->CreateElementContainer(false);
  child->element_container_impl()->InsertSelf();

  auto grandchild = manager->CreateFiberElement("text");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(grandchild.get()));
  child->AddChildAt(grandchild, 0);
  grandchild->CreateElementContainer(false);
  grandchild->element_container_impl()->InsertSelf();

  auto depth_zero =
      lynx::devtool::ElementHelper::GetDocumentBodyFromNode(root.get(), 0);
  EXPECT_EQ(depth_zero["nodeId"],
            lynx::devtool::ElementInspector::NodeId(root.get()));
  EXPECT_EQ(depth_zero["childNodeCount"], 1);
  EXPECT_TRUE(depth_zero["children"].isNull());

  auto depth_one =
      lynx::devtool::ElementHelper::GetDocumentBodyFromNode(root.get(), 1);
  ASSERT_TRUE(depth_one["children"].isArray());
  ASSERT_EQ(depth_one["children"].size(), 1U);
  EXPECT_EQ(depth_one["children"][0]["nodeId"],
            lynx::devtool::ElementInspector::NodeId(child.get()));
  EXPECT_EQ(depth_one["children"][0]["childNodeCount"], 1);
  EXPECT_TRUE(depth_one["children"][0]["children"].isNull());

  auto full_depth =
      lynx::devtool::ElementHelper::GetDocumentBodyFromNode(root.get(), -1);
  ASSERT_TRUE(full_depth["children"].isArray());
  ASSERT_EQ(full_depth["children"].size(), 1U);
  ASSERT_TRUE(full_depth["children"][0]["children"].isArray());
  ASSERT_EQ(full_depth["children"][0]["children"].size(), 1U);
  EXPECT_EQ(full_depth["children"][0]["children"][0]["nodeId"],
            lynx::devtool::ElementInspector::NodeId(grandchild.get()));

  grandchild->element_container_impl()->RemoveSelf(true);
  child->element_container_impl()->RemoveSelf(true);
}

TEST_F(ElementHelperTest, GetElementContentTest) {
  auto element = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));

  std::string content =
      lynx::devtool::ElementHelper::GetElementContent(element.get(), 0);
  EXPECT_EQ(content, "<view>\n</view>\n");

  lynx::devtool::ElementHelper::SetAttributesAsText(element.get(), "style",
                                                    "style='color: pink;'");
  content = lynx::devtool::ElementHelper::GetElementContent(element.get(), 0);
  EXPECT_EQ(content, "<view style=\"'color: pink;'\">\n</view>\n");

  std::string value = "true";
  lynx::devtool::ElementInspector::UpdateAttr(element.get(),
                                              "enable-new-animator", value);
  content = lynx::devtool::ElementHelper::GetElementContent(element.get(), 0);
  EXPECT_EQ(content,
            "<view style=\"'color: pink;'\" "
            "enable-new-animator=\"true\">\n</view>\n");
}

TEST_F(ElementHelperTest, PerformSearchFromNodeTest) {
  auto element = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));

  std::vector<int> results;
  std::string query = "view";
  lynx::devtool::ElementHelper::PerformSearchFromNode(element.get(), query,
                                                      results);
  EXPECT_EQ(results.size(), 1);
  EXPECT_EQ(results[0], 10);

  results.clear();
  query = "text";
  lynx::devtool::ElementHelper::PerformSearchFromNode(element.get(), query,
                                                      results);
  EXPECT_EQ(results.size(), 0);

  // for element tree
  auto element1 = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element1.get()));
  element1->CreateElementContainer(false);

  auto child1 = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child1.get()));
  element1->AddChildAt(child1, 0);

  child1->CreateElementContainer(false);
  auto child_container = child1->element_container_impl();
  child_container->InsertSelf();
  results.clear();
  query = "view";
  lynx::devtool::ElementHelper::PerformSearchFromNode(element1.get(), query,
                                                      results);
  EXPECT_EQ(results.size(), 2);
  EXPECT_EQ(results[0], 11);
  EXPECT_EQ(results[1], 12);
  child_container->RemoveSelf(true);
}

TEST_F(ElementHelperTest, GetStyleSheetAsTextOfNodeTest) {
  Json::Value res =
      lynx::devtool::ElementHelper::GetBackGroundColorsOfNode(nullptr);
  Json::Value error = Json::Value(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  error["code"] = Json::Value(-32000);
  error["message"] = Json::Value("Node is not an Element");
  content["error"] = error;
  EXPECT_EQ(res, content);

  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  lynx::devtool::ElementInspector::UpdateStyle(element.get(), "width", "10px");

  Json::Value res1 = lynx::devtool::ElementHelper::GetStyleSheetAsTextOfNode(
      element.get(), "10", {0, 0, 0, 11});
  std::string expectedStyles = R"({
    "styles" : 
    [
      {
        "cssProperties" : 
        [
          {
            "disabled" : false,
            "implicit" : false,
            "name" : "width",
            "parsedOk" : true,
            "range" : 
            {
              "endColumn" : 11,
              "endLine" : 0,
              "startColumn" : 0,
              "startLine" : 0
            },
            "text" : "width:10px;",
            "value" : "10px"
          }
        ],
        "cssText" : "width:10px;",
        "range" : 
        {
          "endColumn" : 11,
          "endLine" : 0,
          "startColumn" : 0,
          "startLine" : 0
        },
        "shorthandEntries" : [],
        "styleSheetId" : "10"
      }
    ]
  }
  )";
  Json::Reader reader;
  Json::Value json_value;
  reader.parse(expectedStyles, json_value);
  EXPECT_EQ(res1, json_value);
}

TEST_F(ElementHelperTest, UpdateStyleNormalizesImportantSuffix) {
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  // A raw authored value carrying a trailing !important (e.g. from
  // setNativeProps) must be normalized: value_ loses the suffix and important_
  // becomes the single source of truth, matching the parsed path invariant.
  lynx::devtool::ElementInspector::UpdateStyle(element.get(), "color",
                                               "red !important");
  lynx::devtool::ElementInspector::UpdateStyle(element.get(), "width", "10px");

  const auto& inline_sheet =
      lynx::devtool::ElementInspector::GetInlineStyleSheet(element.get());

  auto color_it = inline_sheet.css_properties_.find("color");
  ASSERT_NE(color_it, inline_sheet.css_properties_.end());
  EXPECT_EQ(color_it->second.value_, "red");
  EXPECT_TRUE(color_it->second.important_);
  EXPECT_EQ(color_it->second.text_, "color:red !important;");

  auto width_it = inline_sheet.css_properties_.find("width");
  ASSERT_NE(width_it, inline_sheet.css_properties_.end());
  EXPECT_EQ(width_it->second.value_, "10px");
  EXPECT_FALSE(width_it->second.important_);
  EXPECT_EQ(width_it->second.text_, "width:10px;");
}

TEST_F(ElementHelperTest, GetMatchedStylesForNodeTest) {
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  lynx::devtool::ElementInspector::UpdateStyle(element.get(), "width", "10px");
  Json::Value res =
      lynx::devtool::ElementHelper::GetMatchedStylesForNode(element.get());
  std::string expectedStyles = R"(
  {
    "cssKeyframesRules" : [],
    "inlineStyle" : 
    {
      "cssProperties" : 
      [
        {
          "disabled" : false,
          "implicit" : false,
          "name" : "width",
          "parsedOk" : true,
          "range" : 
          {
            "endColumn" : 11,
            "endLine" : 0,
            "startColumn" : 0,
            "startLine" : 0
          },
          "text" : "width:10px;",
          "value" : "10px"
        }
      ],
      "cssText" : "width:10px;",
      "range" : 
      {
        "endColumn" : 11,
        "endLine" : 0,
        "startColumn" : 0,
        "startLine" : 0
      },
      "shorthandEntries" : [],
      "styleSheetId" : "10"
    },
    "matchedCSSRules" : [],
    "pseudoElements" : []
  })";
  Json::Reader reader;
  Json::Value json_value;
  reader.parse(expectedStyles, json_value);
  EXPECT_EQ(res, json_value);

  lynx::devtool::ElementInspector::UpdateStyle(element.get(), "background",
                                               "pink");
  res = lynx::devtool::ElementHelper::GetMatchedStylesForNode(element.get());
  expectedStyles = R"({
    "cssKeyframesRules" : [],
    "inlineStyle" : 
    {
      "cssProperties" : 
      [
        {
          "disabled" : false,
          "implicit" : false,
          "name" : "width",
          "parsedOk" : true,
          "range" : 
          {
            "endColumn" : 11,
            "endLine" : 0,
            "startColumn" : 0,
            "startLine" : 0
          },
          "text" : "width:10px;",
          "value" : "10px"
        },
        {
          "disabled" : false,
          "implicit" : false,
          "name" : "background",
          "parsedOk" : true,
          "range" : 
          {
            "endColumn" : 27,
            "endLine" : 0,
            "startColumn" : 11,
            "startLine" : 0
          },
          "text" : "background:pink;",
          "value" : "pink"
        }
      ],
      "cssText" : "width:10px;background:pink;",
      "range" : 
      {
        "endColumn" : 27,
        "endLine" : 0,
        "startColumn" : 0,
        "startLine" : 0
      },
      "shorthandEntries" : [],
      "styleSheetId" : "10"
    },
    "matchedCSSRules" : [],
    "pseudoElements" : [] 
  })";

  Json::Value json_value1;
  reader.parse(expectedStyles, json_value1);
  EXPECT_EQ(res, json_value1);
  lynx::devtool::ElementInspector::UpdateStyle(element.get(), "width", "15px");
  res = lynx::devtool::ElementHelper::GetMatchedStylesForNode(element.get());
  expectedStyles = R"({
    "cssKeyframesRules" : [],
    "inlineStyle" : 
    {
      "cssProperties" : 
      [
        {
          "disabled" : false,
          "implicit" : false,
          "name" : "width",
          "parsedOk" : true,
          "range" : 
          {
            "endColumn" : 11,
            "endLine" : 0,
            "startColumn" : 0,
            "startLine" : 0
          },
          "text" : "width:15px;",
          "value" : "15px"
        },
        {
          "disabled" : false,
          "implicit" : false,
          "name" : "background",
          "parsedOk" : true,
          "range" : 
          {
            "endColumn" : 27,
            "endLine" : 0,
            "startColumn" : 11,
            "startLine" : 0
          },
          "text" : "background:pink;",
          "value" : "pink"
        }
      ],
      "cssText" : "width:15px;background:pink;",
      "range" : 
      {
        "endColumn" : 27,
        "endLine" : 0,
        "startColumn" : 0,
        "startLine" : 0
      },
      "shorthandEntries" : [],
      "styleSheetId" : "10"
    },
    "matchedCSSRules" : [],
    "pseudoElements" : []
  })";
  Json::Value json_value2;
  reader.parse(expectedStyles, json_value2);
  EXPECT_EQ(res, json_value2);
}

TEST_F(ElementHelperTest, GetMatchedStylesForNodeReportsActiveMediaRules) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));

  auto fragment = CreateMediaCSSFragment({
      {".active-media",
       lynx::tasm::CSSPropertyID::kPropertyIDWidth,
       "10px",
       "(min-width: 1000px)",
       {"responsive", "desktop"}},
      {".inactive-media", lynx::tasm::CSSPropertyID::kPropertyIDHeight, "20px",
       "(max-width: 10px)"},
  });

  lynx::base::String component_id("21");
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(component_id, 100, entry_name,
                                                 component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      fragment.get(), manager.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto styled_element = manager->CreateFiberElement("view");
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  styled_element->data_model()->SetClass("active-media");
  styled_element->data_model()->SetClass("inactive-media");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));
  component->InsertNode(styled_element);

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));

  Json::Value matched = lynx::devtool::ElementHelper::GetMatchedStylesForNode(
      styled_element.get());

  ASSERT_TRUE(matched["matchedCSSRules"].isArray());
  ASSERT_EQ(matched["matchedCSSRules"].size(), 1U);
  const Json::Value& rule = matched["matchedCSSRules"][0]["rule"];
  EXPECT_EQ(rule["selectorList"]["text"].asString(), ".active-media");
  ASSERT_TRUE(rule["media"].isArray());
  ASSERT_EQ(rule["media"].size(), 1U);
  EXPECT_EQ(rule["media"][0]["text"].asString(), "(min-width: 1000px)");
  EXPECT_EQ(rule["media"][0]["source"].asString(), "mediaRule");
  EXPECT_EQ(rule["media"][0]["styleSheetId"].asString(),
            std::to_string(
                lynx::devtool::ElementInspector::NodeId(style_root.get())));
  EXPECT_EQ(rule["media"][0]["range"]["startLine"].asInt(), 0);
  EXPECT_EQ(rule["media"][0]["range"]["endLine"].asInt(), 0);
  EXPECT_EQ(rule["media"][0]["range"]["startColumn"].asInt(), 0);
  EXPECT_EQ(rule["media"][0]["range"]["endColumn"].asInt(),
            static_cast<int>(std::string("(min-width: 1000px)").length()));
  ASSERT_TRUE(rule["layers"].isArray());
  ASSERT_EQ(rule["layers"].size(), 2U);
  EXPECT_EQ(rule["layers"][0]["text"].asString(), "responsive");
  EXPECT_EQ(rule["layers"][1]["text"].asString(), "desktop");
}

TEST_F(ElementHelperTest, GetMatchedStylesForNodeReportsCascadeLayers) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));

  auto fragment = CreateLayeredCSSFragment(
      ".layered", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px",
      {"framework", "components"}, true);
  fragment->GetOrCreateRootLayer()->GetOrAddSubLayer({"unused"});

  lynx::base::String component_id("21");
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(component_id, 100, entry_name,
                                                 component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      fragment.get(), manager.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto styled_element = manager->CreateFiberElement("view");
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  styled_element->data_model()->SetClass("layered");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));
  component->InsertNode(styled_element);

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));

  Json::Value matched = lynx::devtool::ElementHelper::GetMatchedStylesForNode(
      styled_element.get());

  ASSERT_TRUE(matched["matchedCSSRules"].isArray());
  ASSERT_EQ(matched["matchedCSSRules"].size(), 1U);
  const Json::Value& rule = matched["matchedCSSRules"][0]["rule"];
  EXPECT_EQ(rule["selectorList"]["text"].asString(), ".layered");
  const Json::Value& properties = rule["style"]["cssProperties"];
  ASSERT_EQ(properties.size(), 1U);
  EXPECT_EQ(properties[0]["name"].asString(), "width");
  EXPECT_EQ(properties[0]["value"].asString(), "10px !important");
  EXPECT_TRUE(properties[0]["important"].asBool());
  EXPECT_EQ(properties[0]["text"].asString(), "width:10px !important;");
  ASSERT_TRUE(rule["layers"].isArray());
  ASSERT_EQ(rule["layers"].size(), 2U);
  EXPECT_EQ(rule["layers"][0]["text"].asString(), "framework");
  EXPECT_EQ(rule["layers"][1]["text"].asString(), "components");

  const Json::Value layer_tree =
      lynx::devtool::ElementHelper::GetLayersForNode(styled_element.get());
  const Json::Value& root_layer = layer_tree["rootLayer"];
  EXPECT_EQ(root_layer["name"].asString(), "implicit outer layer");
  EXPECT_EQ(root_layer["order"].asUInt(), 3U);
  ASSERT_EQ(root_layer["subLayers"].size(), 2U);
  EXPECT_EQ(root_layer["subLayers"][0]["name"].asString(), "framework");
  EXPECT_EQ(root_layer["subLayers"][0]["order"].asUInt(), 1U);
  ASSERT_EQ(root_layer["subLayers"][0]["subLayers"].size(), 1U);
  EXPECT_EQ(root_layer["subLayers"][0]["subLayers"][0]["name"].asString(),
            "components");
  EXPECT_EQ(root_layer["subLayers"][0]["subLayers"][0]["order"].asUInt(), 0U);
  EXPECT_EQ(root_layer["subLayers"][1]["name"].asString(), "unused");
  EXPECT_EQ(root_layer["subLayers"][1]["order"].asUInt(), 2U);

  auto initial_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), ".layered");
  ASSERT_FALSE(initial_sheet.empty);
  const std::vector<std::string> expected_layers{"framework", "components"};
  EXPECT_EQ(initial_sheet.layers_, expected_layers);
  ASSERT_EQ(initial_sheet.css_properties_.count("width"), 1U);
  EXPECT_TRUE(initial_sheet.css_properties_.find("width")->second.important_);

  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "width:20px !important;",
                                              initial_sheet.style_value_range_);

  auto edited_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), ".layered");
  EXPECT_EQ(edited_sheet.layers_, expected_layers);

  Json::Value matched_after_edit =
      lynx::devtool::ElementHelper::GetMatchedStylesForNode(
          styled_element.get());
  const Json::Value& layers_after_edit =
      matched_after_edit["matchedCSSRules"][0]["rule"]["layers"];
  ASSERT_EQ(layers_after_edit.size(), 2U);
  EXPECT_EQ(layers_after_edit[0]["text"].asString(), "framework");
  EXPECT_EQ(layers_after_edit[1]["text"].asString(), "components");
  const Json::Value& properties_after_edit =
      matched_after_edit["matchedCSSRules"][0]["rule"]["style"]
                        ["cssProperties"];
  ASSERT_EQ(properties_after_edit.size(), 1U);
  EXPECT_EQ(properties_after_edit[0]["value"].asString(), "20px !important");
  EXPECT_TRUE(properties_after_edit[0]["important"].asBool());
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsLegacyPipelineUpdatesImportantLayeredStyles) {
  manager->enable_new_styling_pipeline_ = false;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  auto fragment = CreateLayeredCascadeCSSFragment(
      ".result--important",
      lynx::tasm::CSSPropertyID::kPropertyIDBackgroundColor,
      {
          {"#262626", {"important", "first"}, true},
          {"#e85d3f", {"important", "second"}, true},
          {"#4a0ccf", {}, true},
      });

  lynx::base::String component_id("21");
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(component_id, 100, entry_name,
                                                 component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      fragment.get(), manager.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto styled_element = manager->CreateFiberElement("view");
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  styled_element->data_model()->SetClass("result--important");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));
  component->InsertNode(styled_element);

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));

  page->FlushActionsAsRoot();

  const std::vector<std::string> first_layer{"important", "first"};
  auto find_first_layer_sheet = [&]() {
    auto styles = lynx::devtool::ElementInspector::GetMatchedStyleSheet(
        styled_element.get());
    auto it = std::find_if(styles.begin(), styles.end(),
                           [&first_layer](const auto& style) {
                             return style.layers_ == first_layer;
                           });
    return it == styles.end() ? lynx::devtool::InspectorStyleSheet() : *it;
  };

  auto first_layer_sheet = find_first_layer_sheet();
  ASSERT_FALSE(first_layer_sheet.empty);
  lynx::devtool::ElementHelper::SetStyleTexts(
      page.get(), style_root.get(), "background-color:#262626;",
      first_layer_sheet.style_value_range_);
  auto& normal_background =
      styled_element->computed_css_style()->GetBackgroundData();
  ASSERT_TRUE(normal_background.has_value());
  EXPECT_EQ(normal_background->color, 0xFFE85D3FU);

  first_layer_sheet = find_first_layer_sheet();
  ASSERT_FALSE(first_layer_sheet.empty);
  lynx::devtool::ElementHelper::SetStyleTexts(
      page.get(), style_root.get(), "background-color:#262626 !important;",
      first_layer_sheet.style_value_range_);
  auto& restored_background =
      styled_element->computed_css_style()->GetBackgroundData();
  ASSERT_TRUE(restored_background.has_value());
  EXPECT_EQ(restored_background->color, 0xff262626U);
}

TEST_F(ElementHelperTest, SetStyleTextsTest) {
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  // origin setting
  lynx::devtool::ElementInspector::UpdateStyle(element.get(), "background",
                                               "pink");
  // set style text, replace origin setting
  lynx::devtool::ElementHelper::SetStyleTexts(nullptr, element.get(),
                                              "width:10px", {0, 0, 0, 0});

  Json::Value res =
      lynx::devtool::ElementHelper::GetMatchedStylesForNode(element.get());
  std::string expectedStyles = R"({
    "cssKeyframesRules" : [],
    "inlineStyle" : 
    {
      "cssProperties" : 
      [
        {
          "disabled" : false,
          "implicit" : false,
          "name" : "width",
          "parsedOk" : true,
          "range" : 
          {
            "endColumn" : 12,
            "endLine" : 0,
            "startColumn" : 1,
            "startLine" : 0
          },
          "text" : "width:10px;",
          "value" : "10px"
        }
      ],
      "cssText" : "width:10px;",
      "range" : 
      {
        "endColumn" : 12,
        "endLine" : 0,
        "startColumn" : 1,
        "startLine" : 0
      },
      "shorthandEntries" : [],
      "styleSheetId" : "10"
    },
    "matchedCSSRules" : [],
    "pseudoElements" : []
  })";
  Json::Value json_value;
  Json::Reader reader;
  reader.parse(expectedStyles, json_value);
  EXPECT_EQ(res, json_value);
}

TEST_F(ElementHelperTest,
       GetInlineStyleOfNodeExcludesMatchedSelectorVariables) {
  // Regression test for the DevTool Styles panel showing wrong / struck-through
  // CSS variable values. Variables resolved from matched selector rules (e.g.
  // .light { --doubao-color: red }) live in css_variables_map(), and the var()
  // names a node references live in css_variable_related(). Neither is an
  // inline declaration. Surfacing them as inline styles fabricates a
  // highest-priority inline declaration that incorrectly strikes through the
  // real matched rule and can display a stale value when the matched rule
  // changes. Only variables declared in the element's own inline style
  // (css_variable_from_js) belong in the inline style payload.
  auto element = manager->CreateFiberElement("view");
  auto* node = element->data_model();
  ASSERT_NE(node, nullptr);

  // Matched selector variable, as produced by DidCollectMatchedRules for
  // `.light { --doubao-color: red }`.
  node->UpdateCSSVariable(lynx::base::String("--doubao-color"),
                          lynx::base::String("red"));
  // var() reference tracking for invalidation (a dependency, not a
  // declaration).
  node->AddCSSVariableRelated(lynx::base::String("--doubao-color"),
                              lynx::base::String("red"));
  // Genuine inline-declared custom property, e.g. style="--inline-brand: blue".
  node->UpdateCSSInlineVariables(lynx::base::String("--inline-brand"),
                                 lynx::base::String("blue"));

  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  Json::Value inline_style =
      lynx::devtool::ElementHelper::GetInlineStyleOfNode(element.get());

  bool found_inline_brand = false;
  bool found_doubao = false;
  for (const auto& prop : inline_style["cssProperties"]) {
    const std::string name = prop["name"].asString();
    if (name == "--inline-brand") {
      found_inline_brand = true;
      EXPECT_EQ(prop["value"].asString(), "blue");
    } else if (name == "--doubao-color") {
      found_doubao = true;
    }
  }
  // The genuinely inline variable is kept; the matched-rule variable is not
  // re-emitted as an inline style.
  EXPECT_TRUE(found_inline_brand);
  EXPECT_FALSE(found_doubao);
  EXPECT_EQ(inline_style["cssText"].asString().find("--doubao-color"),
            std::string::npos);
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsLegacyPipelineSyncsCustomProperties) {
  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  fml::RefPtr<lynx::tasm::CSSParseToken> shared_variable_token;
  fml::RefPtr<lynx::tasm::CSSParseToken> matched_variable_token;
  auto selector_fragment = CreateSplitTokenCustomPropertyCSSFragment(
      ".theme.primary", "--size", "100px", &shared_variable_token,
      &matched_variable_token);
  lynx::tasm::CSSParserConfigs parser_configs;
  auto consumer_token =
      fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  consumer_token->SetAttribute(lynx::tasm::CSSPropertyID::kPropertyIDHeight,
                               ParseVariableValue("var(--size, 10px)"));
  consumer_token->MarkParsed();
  auto consumer_sheet = std::make_shared<lynx::tasm::CSSSheet>(".consumer");
  consumer_token->sheets().emplace_back(consumer_sheet);
  selector_fragment->AddStyleRule(CreateSelectorArray(".consumer"),
                                  consumer_token);
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  lynx::base::String component_id("21");
  int32_t css_id = 100;
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto styled_element = manager->CreateFiberElement("view");
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  styled_element->SetClasses({"theme", "primary"});
  component->InsertNode(styled_element);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));

  auto consumer = manager->CreateFiberElement("view");
  consumer->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  consumer->SetClass("consumer");
  styled_element->InsertNode(consumer);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(consumer.get()));

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(consumer.get(), style_root.get()));

  lynx::tasm::CSSVariableMap initial_variables;
  initial_variables.insert_or_assign(lynx::base::String("--size"),
                                     lynx::base::String("100px"));
  styled_element->data_model()->UpdateCSSVariable(std::move(initial_variables),
                                                  nullptr);
  InspectorStyleSheet initial_consumer_style;
  CSSPropertyDetail initial_height{};
  initial_height.name_ = "height";
  initial_height.value_ = "var(--size, 10px)";
  initial_height.parsed_ok_ = true;
  initial_consumer_style.css_properties_.emplace(initial_height.name_,
                                                 std::move(initial_height));
  lynx::devtool::ElementInspector::SetPropsAccordingToStyleSheet(
      consumer.get(), initial_consumer_style);
  EXPECT_NE(consumer->data_model()->css_variable_related().find(
                lynx::base::String("--size")),
            consumer->data_model()->css_variable_related().end());

  auto matched_styles = lynx::devtool::ElementInspector::GetMatchedStyleSheet(
      styled_element.get());
  ASSERT_EQ(matched_styles.size(), 1U);
  auto initial_sheet = matched_styles[0];
  ASSERT_FALSE(initial_sheet.empty);

  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "--size:200px;",
                                              initial_sheet.style_value_range_);

  const auto& updated_variables =
      styled_element->data_model()->css_variables_map();
  auto size_it = updated_variables.find(lynx::base::String("--size"));
  ASSERT_NE(size_it, updated_variables.end());
  EXPECT_EQ(size_it->second.str(), "200px");

  auto updated_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), ".theme.primary");
  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "/* --size:200px; */",
                                              updated_sheet.style_value_range_);

  auto disabled_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), ".theme.primary");
  auto disabled_size = disabled_sheet.css_properties_.find("--size");
  ASSERT_NE(disabled_size, disabled_sheet.css_properties_.end());
  EXPECT_TRUE(disabled_size->second.disabled_);
  EXPECT_TRUE(matched_variable_token->GetStyleVariables().empty());
  EXPECT_EQ(styled_element->data_model()->css_variables_map().count(
                lynx::base::String("--size")),
            0U);
  lynx::devtool::ElementInspector::RecordStyleSheetSourceToken(
      style_root.get(), disabled_sheet, shared_variable_token);

  lynx::devtool::ElementHelper::SetStyleTexts(
      page.get(), style_root.get(), "--size:200px;",
      disabled_sheet.style_value_range_);

  auto reenabled_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), ".theme.primary");
  auto reenabled_size = reenabled_sheet.css_properties_.find("--size");
  ASSERT_NE(reenabled_size, reenabled_sheet.css_properties_.end());
  EXPECT_FALSE(reenabled_size->second.disabled_);
  EXPECT_TRUE(reenabled_size->second.parsed_ok_);
  auto runtime_size = styled_element->data_model()->css_variables_map().find(
      lynx::base::String("--size"));
  ASSERT_NE(runtime_size,
            styled_element->data_model()->css_variables_map().end());
  EXPECT_EQ(runtime_size->second.str(), "200px");
  auto matched_size = matched_variable_token->GetStyleVariables().find(
      lynx::base::String("--size"));
  ASSERT_NE(matched_size, matched_variable_token->GetStyleVariables().end());
  EXPECT_EQ(matched_size->second.str(), "200px");
  auto shared_size = shared_variable_token->GetStyleVariables().find(
      lynx::base::String("--size"));
  ASSERT_NE(shared_size, shared_variable_token->GetStyleVariables().end());
  EXPECT_EQ(shared_size->second.str(), "100px");
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsNewPipelineReenablesCustomProperties) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  auto selector_fragment =
      CreateCustomPropertyCSSFragment(".theme", "--size", "100px");
  lynx::tasm::CSSParserConfigs parser_configs;
  auto consumer_token =
      fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  consumer_token->SetAttribute(lynx::tasm::CSSPropertyID::kPropertyIDHeight,
                               ParseVariableValue("var(--size, 10px)"));
  consumer_token->MarkParsed();
  auto consumer_sheet = std::make_shared<lynx::tasm::CSSSheet>(".consumer");
  consumer_token->sheets().emplace_back(consumer_sheet);
  selector_fragment->AddStyleRule(CreateSelectorArray(".consumer"),
                                  consumer_token);
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  auto component = manager->CreateFiberComponent(
      lynx::base::String("21"), 100, lynx::base::String("__Card__"),
      lynx::base::String("TestComp"), lynx::base::String("/TestComp"));
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto element = manager->CreateFiberElement("view");
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  element->SetClass("theme");
  component->InsertNode(element);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  auto consumer = manager->CreateFiberElement("view");
  consumer->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  consumer->SetClass("consumer");
  element->InsertNode(consumer);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(consumer.get()));

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(element.get(), style_root.get()));

  auto matched_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(element.get());
  ASSERT_EQ(matched_styles.size(), 1U);
  auto style_sheet = matched_styles[0];

  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "--size:200px;",
                                              style_sheet.style_value_range_);
  auto computed_variables =
      element->computed_css_style()->GetCustomProperties();
  ASSERT_NE(computed_variables, nullptr);
  EXPECT_NE(computed_variables->find(lynx::base::String("--size")),
            computed_variables->end());
  auto consumer_height =
      consumer->computed_css_style()->GetResolvedValues().find(
          lynx::tasm::CSSPropertyID::kPropertyIDHeight);
  ASSERT_NE(consumer_height,
            consumer->computed_css_style()->GetResolvedValues().end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDHeight,
                consumer_height->second),
            "200px");
  style_sheet =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(element.get())[0];
  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "/* --size:200px; */",
                                              style_sheet.style_value_range_);
  computed_variables = element->computed_css_style()->GetCustomProperties();
  EXPECT_TRUE(computed_variables == nullptr ||
              computed_variables->find(lynx::base::String("--size")) ==
                  computed_variables->end());
  consumer_height = consumer->computed_css_style()->GetResolvedValues().find(
      lynx::tasm::CSSPropertyID::kPropertyIDHeight);
  ASSERT_NE(consumer_height,
            consumer->computed_css_style()->GetResolvedValues().end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDHeight,
                consumer_height->second),
            "10px");
  style_sheet =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(element.get())[0];
  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "--size:200px;",
                                              style_sheet.style_value_range_);
  computed_variables = element->computed_css_style()->GetCustomProperties();
  ASSERT_NE(computed_variables, nullptr);
  EXPECT_NE(computed_variables->find(lynx::base::String("--size")),
            computed_variables->end());
  consumer_height = consumer->computed_css_style()->GetResolvedValues().find(
      lynx::tasm::CSSPropertyID::kPropertyIDHeight);
  ASSERT_NE(consumer_height,
            consumer->computed_css_style()->GetResolvedValues().end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDHeight,
                consumer_height->second),
            "200px");

  auto token = selector_fragment->GetSharedCSSStyle(".theme");
  ASSERT_NE(token, nullptr);
  auto variable = token->GetStyleVariables().find(lynx::base::String("--size"));
  ASSERT_NE(variable, token->GetStyleVariables().end());
  EXPECT_EQ(variable->second.str(), "200px");
  auto runtime_variable = element->data_model()->css_variables_map().find(
      lynx::base::String("--size"));
  ASSERT_NE(runtime_variable, element->data_model()->css_variables_map().end());
  EXPECT_EQ(runtime_variable->second.str(), "200px");
}

TEST_F(ElementHelperTest,
       UpdateMatchedCSSVariablesForDevToolInvalidatesEqualVariableValue) {
  auto parent = manager->CreateFiberElement("view");
  auto child = manager->CreateFiberElement("view");
  parent->InsertNode(child);

  lynx::tasm::CSSVariableMap variables;
  variables.insert_or_assign(lynx::base::String("--brand"),
                             lynx::base::String("blue"));
  parent->data_model()->UpdateCSSVariable(variables, nullptr);
  ASSERT_TRUE(child->CollectCustomProperties(child->data_model()));
  ASSERT_NE(child->GetInheritedProperty().custom_properties_, nullptr);

  parent->dirty_ = 0;
  child->dirty_ = 0;
  parent->UpdateMatchedCSSVariablesForDevTool(std::move(variables));

  EXPECT_TRUE(parent->dirty_ & lynx::tasm::Element::kDirtyRefreshCSSVariables);
  EXPECT_TRUE(child->StyleDirty());
  EXPECT_EQ(child->GetInheritedProperty().custom_properties_, nullptr);
}

TEST_F(ElementHelperTest,
       SetPropsAccordingToStyleSheetResolvesCSSVariableValue) {
  auto parent = manager->CreateFiberElement("view");
  auto element = manager->CreateFiberElement("view");
  parent->InsertNode(element);
  lynx::tasm::CSSVariableMap variables;
  variables.insert_or_assign(lynx::base::String("--main-height"),
                             lynx::base::String("100px"));
  parent->data_model()->UpdateCSSVariable(std::move(variables), nullptr);
  ASSERT_EQ(element->data_model()
                ->GetCSSVariableValue(lynx::base::String("--main-height"))
                .str(),
            "100px");

  InspectorStyleSheet style_sheet;
  CSSPropertyDetail height{};
  height.name_ = "height";
  height.value_ = "var(--main-height)";
  height.parsed_ok_ = true;
  style_sheet.css_properties_.emplace(height.name_, std::move(height));

  ElementInspector::SetPropsAccordingToStyleSheet(element.get(), style_sheet);

  const auto& related = element->data_model()->css_variable_related();
  auto height_variable = related.find(lynx::base::String("--main-height"));
  ASSERT_NE(height_variable, related.end());
  EXPECT_EQ(height_variable->second.str(), "100px");

  CSSVariableSnapshot variable_snapshot;
  lynx::tasm::CSSVariableMap edited_variables;
  edited_variables.insert_or_assign(lynx::base::String("--main-height"),
                                    lynx::base::String("200px"));
  variable_snapshot.insert_or_assign(parent->data_model(),
                                     std::move(edited_variables));
  ElementInspector::SetPropsAccordingToStyleSheet(element.get(), style_sheet,
                                                  &variable_snapshot);

  height_variable = related.find(lynx::base::String("--main-height"));
  ASSERT_NE(height_variable, related.end());
  EXPECT_EQ(height_variable->second.str(), "200px");
  EXPECT_EQ(element->data_model()
                ->GetCSSVariableValue(lynx::base::String("--main-height"))
                .str(),
            "100px");
}

TEST_F(ElementHelperTest,
       SetInlineStyleTextsNewPipelineSyncsAndClearsInlineVariables) {
  manager->enable_new_styling_pipeline_ = true;
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  auto page = manager->CreateFiberPage("page", 0);

  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  page->InsertNode(element);

  lynx::devtool::ElementHelper::SetInlineStyleTexts(
      element.get(), "--brand:red;color:var(--brand)", Range(), page.get());

  EXPECT_EQ(lynx::devtool::ElementInspector::GetInlineStyleSheet(element.get())
                .css_text_,
            "--brand:red;color:var(--brand);");
  EXPECT_TRUE(element->GetRawInlineStyles().str().empty());
  auto inline_vars = element->data_model()->GetCSSInlineVariables();
  auto brand_it = inline_vars.find(lynx::base::String("--brand"));
  ASSERT_TRUE(brand_it != inline_vars.end());
  EXPECT_EQ(brand_it->second.str(), "red");

  lynx::devtool::ElementHelper::RemoveAttributes(element.get(), "style");

  EXPECT_TRUE(element->GetRawInlineStyles().str().empty());
  EXPECT_TRUE(element->data_model()->GetCSSInlineVariables().empty());
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsNewPipelineKeepsLastDuplicateProperty) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  auto selector_fragment = CreateSelectorCSSFragment(
      "view", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px");
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  lynx::base::String component_id("21");
  int32_t css_id = 100;
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto styled_element = manager->CreateFiberElement("view");
  auto styled_comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  styled_element->SetAttributeHolder(styled_comp->attribute_holder());
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  component->InsertNode(styled_element);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));

  auto initial_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), "view");
  ASSERT_FALSE(initial_sheet.empty);

  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "width:10px;width:20px;",
                                              initial_sheet.style_value_range_);

  auto token =
      styled_element->GetRelatedCSSFragment()->GetSharedCSSStyle("view");
  ASSERT_NE(token, nullptr);

  const auto& attributes = token->GetAttributes();
  auto width_it = attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != attributes.end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDWidth, width_it->second),
            "20px");
}

TEST_F(ElementHelperTest,
       StyleSheetSourceTokenCacheKeepsTokenAliveUntilStyleRootDestroyed) {
  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));

  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.empty = false;
  style_sheet.style_name_ = ".owned";
  style_sheet.position_ = 1;
  style_sheet.style_value_range_ = {1, 1, 0, 6};

  bool destroyed = false;
  {
    lynx::tasm::CSSParserConfigs parser_configs;
    auto token = fml::MakeRefCounted<LifetimeObservedCSSParseToken>(
        parser_configs, &destroyed);
    lynx::devtool::ElementInspector::RecordStyleSheetSourceToken(
        style_root.get(), style_sheet, token);
    ASSERT_NE(lynx::devtool::ElementInspector::GetStyleSheetSourceToken(
                  style_root.get(), style_sheet),
              nullptr);
  }

  EXPECT_FALSE(destroyed);
  style_root = nullptr;
  EXPECT_TRUE(destroyed);
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsNewPipelineUpdatesDescendantSelectorToken) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  fml::RefPtr<lynx::tasm::CSSParseToken> shared_token;
  fml::RefPtr<lynx::tasm::CSSParseToken> matched_token;
  auto selector_fragment = CreateSplitTokenSelectorCSSFragment(
      ".card .title", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px",
      &shared_token, &matched_token);
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  lynx::base::String component_id("21");
  int32_t css_id = 100;
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto parent = manager->CreateFiberElement("view");
  parent->data_model()->SetClass("card");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(parent.get()));
  component->InsertNode(parent);

  auto styled_element = manager->CreateFiberElement("view");
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  styled_element->data_model()->SetClass("title");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));
  parent->InsertNode(styled_element);

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));

  auto matched_styles = lynx::devtool::ElementInspector::GetMatchedStyleSheet(
      styled_element.get());
  ASSERT_EQ(matched_styles.size(), 1U);
  EXPECT_EQ(matched_styles[0].style_name_, ".card .title");

  auto initial_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), ".card .title");
  ASSERT_FALSE(initial_sheet.empty);

  ASSERT_NE(shared_token, nullptr);
  ASSERT_NE(matched_token, nullptr);
  ASSERT_NE(shared_token.get(), matched_token.get());
  auto stale_token =
      CreateParseToken(lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px");
  lynx::devtool::ElementInspector::RecordStyleSheetSourceToken(
      style_root.get(), initial_sheet, stale_token);

  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "width:20px;",
                                              initial_sheet.style_value_range_);

  const auto& attributes = matched_token->GetAttributes();
  auto width_it = attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != attributes.end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDWidth, width_it->second),
            "20px");
  const auto& shared_attributes = shared_token->GetAttributes();
  auto shared_width =
      shared_attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(shared_width != shared_attributes.end());
  EXPECT_EQ(
      lynx::tasm::CSSDecoder::CSSValueToString(
          lynx::tasm::CSSPropertyID::kPropertyIDWidth, shared_width->second),
      "10px");
  const auto& stale_attributes = stale_token->GetAttributes();
  auto stale_width =
      stale_attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(stale_width != stale_attributes.end());
  EXPECT_EQ(
      lynx::tasm::CSSDecoder::CSSValueToString(
          lynx::tasm::CSSPropertyID::kPropertyIDWidth, stale_width->second),
      "10px");
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsNewPipelineKeepsComponentSelectorTokenIsolated) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));

  auto first_fragment = CreateSelectorCSSFragment(
      ".title", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px");
  auto second_fragment = CreateSelectorCSSFragment(
      ".title", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "40px");

  lynx::base::String first_component_id("21");
  lynx::base::String first_entry_name("__CardA__");
  lynx::base::String first_component_name("TestCompA");
  lynx::base::String first_path("/index/components/TestCompA");
  auto first_component =
      manager->CreateFiberComponent(first_component_id, 100, first_entry_name,
                                    first_component_name, first_path);
  first_component->style_sheet_ =
      std::make_unique<lynx::tasm::CSSFragmentDecorator>(first_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(first_component.get()));
  page->InsertNode(first_component);

  auto first_element = manager->CreateFiberElement("view");
  first_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(first_component->impl_id()));
  first_element->data_model()->SetClass("title");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(first_element.get()));
  first_component->InsertNode(first_element);

  lynx::base::String second_component_id("22");
  lynx::base::String second_entry_name("__CardB__");
  lynx::base::String second_component_name("TestCompB");
  lynx::base::String second_path("/index/components/TestCompB");
  auto second_component =
      manager->CreateFiberComponent(second_component_id, 200, second_entry_name,
                                    second_component_name, second_path);
  second_component->style_sheet_ =
      std::make_unique<lynx::tasm::CSSFragmentDecorator>(second_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(second_component.get()));
  page->InsertNode(second_component);

  auto second_element = manager->CreateFiberElement("view");
  second_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(second_component->impl_id()));
  second_element->data_model()->SetClass("title");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(second_element.get()));
  second_component->InsertNode(second_element);

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(first_element.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(second_element.get(), style_root.get()));

  auto matched_styles = lynx::devtool::ElementInspector::GetMatchedStyleSheet(
      first_element.get());
  ASSERT_EQ(matched_styles.size(), 1U);
  EXPECT_EQ(matched_styles[0].style_name_, ".title");
  auto second_matched_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(
          second_element.get());
  ASSERT_EQ(second_matched_styles.size(), 1U);
  EXPECT_EQ(second_matched_styles[0].style_name_, ".title");
  EXPECT_NE(matched_styles[0].style_value_range_.start_line_,
            second_matched_styles[0].style_value_range_.start_line_);

  lynx::devtool::ElementHelper::SetStyleTexts(
      page.get(), style_root.get(), "width:20px;",
      matched_styles[0].style_value_range_);

  auto first_token =
      first_element->GetRelatedCSSFragment()->GetSharedCSSStyle(".title");
  ASSERT_NE(first_token, nullptr);
  auto second_token =
      second_element->GetRelatedCSSFragment()->GetSharedCSSStyle(".title");
  ASSERT_NE(second_token, nullptr);

  const auto& first_attributes = first_token->GetAttributes();
  auto first_width_it =
      first_attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(first_width_it != first_attributes.end());
  EXPECT_EQ(
      lynx::tasm::CSSDecoder::CSSValueToString(
          lynx::tasm::CSSPropertyID::kPropertyIDWidth, first_width_it->second),
      "20px");

  const auto& second_attributes = second_token->GetAttributes();
  auto second_width_it =
      second_attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(second_width_it != second_attributes.end());
  EXPECT_EQ(
      lynx::tasm::CSSDecoder::CSSValueToString(
          lynx::tasm::CSSPropertyID::kPropertyIDWidth, second_width_it->second),
      "40px");

  auto first_updated_sheet =
      lynx::devtool::ElementInspector::GetStyleSheetByName(first_element.get(),
                                                           ".title");
  ASSERT_FALSE(first_updated_sheet.empty);
  EXPECT_EQ(first_updated_sheet.css_text_, "width:20px;");
  EXPECT_EQ(first_updated_sheet.style_value_range_.start_line_,
            matched_styles[0].style_value_range_.start_line_);
  auto first_recorded_token =
      lynx::devtool::ElementInspector::GetStyleSheetSourceToken(
          style_root.get(), first_updated_sheet);
  ASSERT_NE(first_recorded_token, nullptr);
  EXPECT_EQ(first_recorded_token.get(), first_token.get());

  auto second_updated_sheet =
      lynx::devtool::ElementInspector::GetStyleSheetByName(second_element.get(),
                                                           ".title");
  ASSERT_FALSE(second_updated_sheet.empty);
  EXPECT_EQ(second_updated_sheet.css_text_, "width:40px;");
  EXPECT_EQ(second_updated_sheet.style_value_range_.start_line_,
            second_matched_styles[0].style_value_range_.start_line_);
  auto second_recorded_token =
      lynx::devtool::ElementInspector::GetStyleSheetSourceToken(
          style_root.get(), second_updated_sheet);
  ASSERT_NE(second_recorded_token, nullptr);
  EXPECT_EQ(second_recorded_token.get(), second_token.get());

  lynx::devtool::ElementHelper::SetStyleTexts(
      page.get(), style_root.get(), "width:50px;",
      second_updated_sheet.style_value_range_);

  first_width_it = first_token->GetAttributes().find(
      lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(first_width_it != first_token->GetAttributes().end());
  EXPECT_EQ(
      lynx::tasm::CSSDecoder::CSSValueToString(
          lynx::tasm::CSSPropertyID::kPropertyIDWidth, first_width_it->second),
      "20px");
  second_width_it = second_token->GetAttributes().find(
      lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(second_width_it != second_token->GetAttributes().end());
  EXPECT_EQ(
      lynx::tasm::CSSDecoder::CSSValueToString(
          lynx::tasm::CSSPropertyID::kPropertyIDWidth, second_width_it->second),
      "50px");
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsNewPipelineUpdatesAdoptedSelectorToken) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));

  lynx::tasm::CSSParserConfigs parser_configs;
  auto adopted_token =
      fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  adopted_token->raw_attributes_[lynx::tasm::CSSPropertyID::kPropertyIDWidth] =
      lynx::tasm::CSSValue::MakePlainString("10px");

  auto sheet = std::make_shared<lynx::tasm::CSSSheet>(".adopted");
  adopted_token->sheets().emplace_back(sheet);

  lynx::tasm::CSSParserTokenMap token_map;
  token_map.insert(std::make_pair(".adopted", adopted_token));

  auto adopted_fragment = std::make_unique<lynx::tasm::SharedCSSFragment>(
      1, std::vector<int32_t>{}, token_map, lynx::tasm::CSSKeyframesTokenMap{},
      lynx::tasm::CSSFontFaceRuleMap{});
  adopted_fragment->SetEnableCSSSelector();
  adopted_fragment->SetEnableCSSRule(true);

  lynx::css::CSSParserContext context;
  lynx::css::CSSTokenizer tokenizer(".adopted");
  auto parser_tokens = tokenizer.TokenizeToEOF();
  lynx::css::CSSParserTokenRange range(parser_tokens);
  auto selector_vector =
      lynx::css::CSSSelectorParser::ParseSelector(range, &context);
  size_t flattened_size =
      lynx::css::CSSSelectorParser::FlattenedSize(selector_vector);
  auto selector_arr =
      std::make_unique<lynx::css::LynxCSSSelector[]>(flattened_size);
  lynx::css::CSSSelectorParser::AdoptSelectorVector(
      selector_vector, selector_arr.get(), flattened_size);
  auto* adopted_layer =
      adopted_fragment->GetOrCreateRootLayer()->GetOrAddSubLayer(
          {"runtime", "theme"});
  adopted_fragment->AddStyleRule(fml::MakeRefCounted<lynx::css::StyleRule>(
                                     std::move(selector_arr), adopted_token),
                                 adopted_layer);

  auto wrapper = fml::AdoptRef(
      new lynx::tasm::SharedCSSFragmentWrapper(std::move(adopted_fragment)));
  manager->AdoptStyleSheet(wrapper);

  auto element = manager->CreateFiberElement("view");
  element->data_model()->SetClass("adopted");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  page->InsertNode(element);

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(element.get(), style_root.get()));

  auto matched_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(element.get());
  ASSERT_EQ(matched_styles.size(), 1U);
  EXPECT_EQ(matched_styles[0].style_name_, ".adopted");
  const std::vector<std::string> expected_layers{"runtime", "theme"};
  EXPECT_EQ(matched_styles[0].layers_, expected_layers);

  Json::Value matched =
      lynx::devtool::ElementHelper::GetMatchedStylesForNode(element.get());
  const Json::Value& layers = matched["matchedCSSRules"][0]["rule"]["layers"];
  ASSERT_EQ(layers.size(), 2U);
  EXPECT_EQ(layers[0]["text"].asString(), "runtime");
  EXPECT_EQ(layers[1]["text"].asString(), "theme");

  auto initial_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      element.get(), ".adopted");
  ASSERT_FALSE(initial_sheet.empty);

  EXPECT_EQ(element->GetRelatedCSSFragment()->GetSharedCSSStyle(".adopted"),
            nullptr);

  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "width:20px;",
                                              initial_sheet.style_value_range_);

  const auto& attributes = adopted_token->GetAttributes();
  auto width_it = attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != attributes.end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDWidth, width_it->second),
            "20px");

  Json::Value matched_after_edit =
      lynx::devtool::ElementHelper::GetMatchedStylesForNode(element.get());
  const Json::Value& layers_after_edit =
      matched_after_edit["matchedCSSRules"][0]["rule"]["layers"];
  ASSERT_EQ(layers_after_edit.size(), 2U);
  EXPECT_EQ(layers_after_edit[0]["text"].asString(), "runtime");
  EXPECT_EQ(layers_after_edit[1]["text"].asString(), "theme");
}

TEST_F(ElementHelperTest,
       GetCssByStyleMapNewPipelineUsesResolvedCustomProperties) {
  manager->enable_new_styling_pipeline_ = true;

  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  auto* style = element->base_css_style() ? element->base_css_style()
                                          : element->computed_css_style();
  style->SetCustomProperty(lynx::base::String("--font"),
                           lynx::tasm::CSSValue::MakePlainString("PingFang"));
  style->FinalizeCustomProperties();

  lynx::tasm::StyleMap style_map;
  style_map.insert_or_assign(lynx::tasm::CSSPropertyID::kPropertyIDFontFamily,
                             ParseVariableValue("var(--font)"));

  auto css = lynx::devtool::ElementInspector::GetCssByStyleMap(element.get(),
                                                               style_map);
  auto font_it = css.find("font-family");
  ASSERT_TRUE(font_it != css.end());
  EXPECT_EQ(font_it->second, "PingFang");

  CSSVariableSnapshot variable_snapshot;
  lynx::tasm::CSSVariableMap devtool_variables;
  devtool_variables.insert_or_assign(lynx::base::String("--font"),
                                     lynx::base::String("DevToolFont"));
  variable_snapshot.insert_or_assign(element->data_model(),
                                     std::move(devtool_variables));
  css = lynx::devtool::ElementInspector::GetCssByStyleMap(
      element.get(), style_map, &variable_snapshot);
  font_it = css.find("font-family");
  ASSERT_TRUE(font_it != css.end());
  EXPECT_EQ(font_it->second, "DevToolFont");
}

TEST_F(ElementHelperTest, InitInlineStyleSheetForInspectorTest) {
  auto element = manager->CreateFiberElement("view");
  element->data_model()->SetInlineStyle(
      lynx::tasm::CSSPropertyID::kPropertyIDFontSize,
      lynx::tasm::CSSValue::MakePlainString("32rpx"));

  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));
  auto style_sheet =
      lynx::devtool::ElementInspector::GetInlineStyleSheet(element.get());

  ASSERT_FALSE(style_sheet.empty);
  EXPECT_EQ(style_sheet.style_sheet_id_, std::to_string(element->impl_id()));
  EXPECT_EQ(style_sheet.style_name_, "inline10");
  EXPECT_EQ(style_sheet.origin_, "regular");
  EXPECT_EQ(style_sheet.css_text_, "font-size:32rpx;");
  ASSERT_EQ(style_sheet.css_properties_.count("font-size"), 1U);
  const auto& property = style_sheet.css_properties_.find("font-size")->second;
  EXPECT_EQ(property.value_, "32rpx");
  EXPECT_EQ(property.text_, "font-size:32rpx;");
  EXPECT_EQ(style_sheet.style_name_range_.start_line_, 0);
  EXPECT_EQ(style_sheet.style_value_range_.start_line_, 0);
  EXPECT_EQ(style_sheet.position_, 0U);
}

TEST_F(ElementHelperTest, InitInlineStyleSheetForInspectorPreservesImportant) {
  auto element = manager->CreateFiberElement("view");
  element->SetRawInlineStyles(
      lynx::base::String("color:red !important;width:10px;"));
  element->ProcessFullRawInlineStyle(nullptr);

  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));
  const auto& style_sheet =
      lynx::devtool::ElementInspector::GetInlineStyleSheet(element.get());

  auto color = style_sheet.css_properties_.find("color");
  ASSERT_NE(color, style_sheet.css_properties_.end());
  EXPECT_EQ(color->second.value_, "red");
  EXPECT_TRUE(color->second.important_);
  EXPECT_EQ(color->second.text_, "color:red !important;");

  auto width = style_sheet.css_properties_.find("width");
  ASSERT_NE(width, style_sheet.css_properties_.end());
  EXPECT_EQ(width->second.value_, "10px");
  EXPECT_FALSE(width->second.important_);
}

TEST_F(ElementHelperTest, GetBackGroundColorsOfNodeTest) {
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));

  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));

  Json::Value res =
      lynx::devtool::ElementHelper::GetBackGroundColorsOfNode(nullptr);
  Json::Value error = Json::Value(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  error["code"] = Json::Value(-32000);
  error["message"] = Json::Value("Node is not an Element");
  content["error"] = error;
  EXPECT_EQ(res, content);

  Json::Value res2 =
      lynx::devtool::ElementHelper::GetBackGroundColorsOfNode(element.get());
  Json::Value content2 = Json::Value(Json::ValueType::objectValue);
  Json::Value backgroundColors = Json::Value(Json::ValueType::arrayValue);
  backgroundColors.append("transparent");
  content2["backgroundColors"] = backgroundColors;
  content2["computedFontSize"] = "medium";
  content2["computedFontWeight"] = "normal";
  EXPECT_EQ(res2, content2);
}

TEST_F(ElementHelperTest, GetOrderedInspectorResetStyleNamesTest) {
  auto reset_names =
      lynx::devtool::ElementInspector::GetOrderedInspectorResetStyleNames();
  ASSERT_FALSE(reset_names.empty());

  for (size_t i = 1; i < reset_names.size(); ++i) {
    EXPECT_LT(static_cast<int>(reset_names[i - 1]),
              static_cast<int>(reset_names[i]));
  }

  auto find_style = [&reset_names](lynx::tasm::CSSPropertyID id) {
    return std::find(reset_names.begin(), reset_names.end(), id);
  };
  auto caret_gradient =
      find_style(lynx::tasm::CSSPropertyID::kPropertyIDXCaretGradient);
  auto caret_width =
      find_style(lynx::tasm::CSSPropertyID::kPropertyIDXCaretWidth);
  auto caret_height =
      find_style(lynx::tasm::CSSPropertyID::kPropertyIDXCaretHeight);
  auto caret_radius =
      find_style(lynx::tasm::CSSPropertyID::kPropertyIDXCaretRadius);

  ASSERT_NE(caret_gradient, reset_names.end());
  ASSERT_NE(caret_width, reset_names.end());
  ASSERT_NE(caret_height, reset_names.end());
  ASSERT_NE(caret_radius, reset_names.end());
  EXPECT_LT(caret_gradient, caret_width);
  EXPECT_LT(caret_width, caret_height);
  EXPECT_LT(caret_height, caret_radius);
}

TEST_F(ElementHelperTest, CreateStyleSheetTest) {
  auto element = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));

  Json::Value res =
      lynx::devtool::ElementHelper::CreateStyleSheet(element.get());
  Json::Value header = Json::Value(Json::ValueType::objectValue);
  header["styleSheetId"] =
      std::to_string(ElementInspector::NodeId(element.get()));
  header["origin"] = "inspector";
  header["sourceURL"] = "";
  header["title"] = "";
  header["ownerNode"] = ElementInspector::NodeId(element.get());
  header["disabled"] = false;
  header["isInline"] = false;
  header["startLine"] = 0;
  header["startColumn"] = 0;
  header["endLine"] = 0;
  header["endColumn"] = 0;
  header["length"] = 0;
  EXPECT_EQ(res, header);
}

TEST_F(ElementHelperTest, SetAttributesAsTextTest) {
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));
  std::vector<Json::Value> res =
      lynx::devtool::ElementHelper::SetAttributesAsText(element.get(), "style",
                                                        "style='color: pink;'");

  for (auto it : res) {
    if (it["method"] == "DOM.attributeModified") {
      EXPECT_EQ(it["params"]["name"], "style");
      EXPECT_EQ(it["params"]["nodeId"], 10);
      EXPECT_EQ(it["params"]["value"], "'color: pink;'");
    } else if (it["method"] == "CSS.styleSheetChanged") {
      EXPECT_EQ(it["params"]["styleSheetId"], "10");
    }
  }
}

TEST_F(ElementHelperTest, GetInheritedCSSRulesOfNodeTest) {
  auto grand_parent = manager->CreateFiberElement("view");
  auto comp1 = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  grand_parent->SetAttributeHolder(comp1->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(grand_parent.get()));

  auto parent = manager->CreateFiberElement("view");
  auto comp2 = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  parent->SetAttributeHolder(comp2->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(parent.get()));

  auto child = manager->CreateFiberElement("view");
  auto comp3 = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  child->SetAttributeHolder(comp3->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));

  grand_parent->AddChildAt(parent, 0);
  parent->AddChildAt(child, 0);

  // Set inline style for grand_parent
  lynx::devtool::ElementHelper::SetAttributesAsText(grand_parent.get(), "style",
                                                    "style=height: 200px;");

  // Set inline style for parent
  lynx::devtool::ElementHelper::SetAttributesAsText(parent.get(), "style",
                                                    "style=width: 100px;");

  Json::Value res =
      lynx::devtool::ElementHelper::GetInheritedCSSRulesOfNode(child.get());

  EXPECT_TRUE(res.isArray());
  EXPECT_EQ(res.size(), 2);  // Two parents (parent and grand_parent)

  // res[0] is parent (closest)
  Json::Value parent_inherited = res[0];
  EXPECT_TRUE(parent_inherited.isMember("inlineStyle"));
  Json::Value parent_inlineStyle = parent_inherited["inlineStyle"];
  EXPECT_TRUE(parent_inlineStyle.isMember("cssProperties"));
  EXPECT_TRUE(parent_inlineStyle["cssProperties"].isArray());
  EXPECT_EQ(parent_inlineStyle["cssProperties"].size(), 1);
  EXPECT_EQ(parent_inlineStyle["cssProperties"][0]["name"].asString(), "width");
  EXPECT_EQ(parent_inlineStyle["cssProperties"][0]["value"].asString(),
            "100px");

  // res[1] is grand_parent
  Json::Value grand_parent_inherited = res[1];
  EXPECT_TRUE(grand_parent_inherited.isMember("inlineStyle"));
  Json::Value grand_parent_inlineStyle = grand_parent_inherited["inlineStyle"];
  EXPECT_TRUE(grand_parent_inlineStyle.isMember("cssProperties"));
  EXPECT_TRUE(grand_parent_inlineStyle["cssProperties"].isArray());
  EXPECT_EQ(grand_parent_inlineStyle["cssProperties"].size(), 1);
  EXPECT_EQ(grand_parent_inlineStyle["cssProperties"][0]["name"].asString(),
            "height");
  EXPECT_EQ(grand_parent_inlineStyle["cssProperties"][0]["value"].asString(),
            "200px");
}

TEST_F(ElementHelperTest, GetMatchedStyleSheetTest) {
  // Test case 1: Basic element with no styles
  auto element = manager->CreateFiberElement("view");
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  element->SetAttributeHolder(comp->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  // Get matched style sheets for element with no styles
  auto res1 =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(element.get());
  // Should return an empty vector since no styles are set
  EXPECT_TRUE(res1.empty());

  // Test case 2: Element with matched selector stylesheet
  auto page = manager->CreateFiberPage("page", 0);
  auto selector_fragment = CreateSelectorCSSFragment(
      "view", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px");
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  lynx::base::String component_id("21");
  int32_t css_id = 100;
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  page->InsertNode(component);

  auto styled_element = manager->CreateFiberElement("view");
  auto styled_comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  styled_element->SetAttributeHolder(styled_comp->attribute_holder());
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  component->InsertNode(styled_element);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));

  ASSERT_NE(styled_element->GetRelatedCSSFragment(), nullptr);

  auto res2 = lynx::devtool::ElementInspector::GetMatchedStyleSheet(
      styled_element.get());
  ASSERT_EQ(res2.size(), 1U);
  EXPECT_EQ(res2[0].style_name_, "view");
  EXPECT_EQ(res2[0].css_text_, "width:10px;");
  auto width_iter = res2[0].css_properties_.find("width");
  ASSERT_NE(width_iter, res2[0].css_properties_.end());
  EXPECT_EQ(width_iter->second.value_, "10px");

  auto variable_token = selector_fragment->GetSharedCSSStyle("view");
  ASSERT_NE(variable_token, nullptr);
  lynx::tasm::StyleMap variable_attributes;
  variable_attributes.insert_or_assign(
      lynx::tasm::CSSPropertyID::kPropertyIDHeight,
      ParseVariableValue("var(--main-height)"));
  variable_token->SetAttributes(std::move(variable_attributes));
  lynx::devtool::ElementInspector::GetStyleSheetMap(style_root.get()).clear();

  lynx::tasm::CSSVariableMap variables;
  variables.insert_or_assign(lynx::base::String("--main-height"),
                             lynx::base::String("200px"));
  styled_element->data_model()->UpdateCSSVariable(std::move(variables),
                                                  nullptr);
  auto initial_variable_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(
          styled_element.get());
  ASSERT_EQ(initial_variable_styles.size(), 1U);
  auto initial_height =
      initial_variable_styles[0].css_properties_.find("height");
  ASSERT_NE(initial_height, initial_variable_styles[0].css_properties_.end());
  EXPECT_EQ(initial_height->second.value_, "var(--main-height)");
  EXPECT_EQ(initial_height->second.text_, "height:var(--main-height);");
  auto initial_computed_style =
      lynx::devtool::ElementInspector::ResolveStyleSheetForComputedStyle(
          styled_element.get(), initial_variable_styles[0]);
  EXPECT_EQ(
      initial_computed_style.css_properties_.find("height")->second.value_,
      "200px");

  variables.insert_or_assign(lynx::base::String("--main-height"),
                             lynx::base::String("300px"));
  styled_element->data_model()->UpdateCSSVariable(std::move(variables),
                                                  nullptr);
  auto updated_variable_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(
          styled_element.get());
  ASSERT_EQ(updated_variable_styles.size(), 1U);
  auto updated_height =
      updated_variable_styles[0].css_properties_.find("height");
  ASSERT_NE(updated_height, updated_variable_styles[0].css_properties_.end());
  EXPECT_EQ(updated_height->second.value_, "var(--main-height)");
  EXPECT_EQ(updated_height->second.text_, "height:var(--main-height);");
  auto updated_computed_style =
      lynx::devtool::ElementInspector::ResolveStyleSheetForComputedStyle(
          styled_element.get(), updated_variable_styles[0]);
  EXPECT_EQ(
      updated_computed_style.css_properties_.find("height")->second.value_,
      "300px");

  lynx::tasm::CSSValue legacy_variable("{{--main-height}}",
                                       lynx::tasm::CSSValuePattern::STRING,
                                       lynx::tasm::CSSValueType::VARIABLE);
  legacy_variable.SetDefaultValue(lynx::base::String("120px"));
  lynx::lepus::Value legacy_default_map(lynx::lepus::Dictionary::Create());
  legacy_default_map.Table()->SetValue("--main-height",
                                       lynx::lepus::Value("120px"));
  legacy_variable.SetDefaultValueMap(std::move(legacy_default_map));
  ASSERT_TRUE(legacy_variable.ToVarReference());
  ASSERT_TRUE(legacy_variable.NeedsVariableResolution());
  variable_attributes.clear();
  variable_attributes.insert_or_assign(
      lynx::tasm::CSSPropertyID::kPropertyIDHeight, std::move(legacy_variable));
  variable_token->SetAttributes(std::move(variable_attributes));
  lynx::devtool::ElementInspector::GetStyleSheetMap(style_root.get()).clear();

  auto legacy_variable_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(
          styled_element.get());
  ASSERT_EQ(legacy_variable_styles.size(), 1U);
  auto legacy_height = legacy_variable_styles[0].css_properties_.find("height");
  ASSERT_NE(legacy_height, legacy_variable_styles[0].css_properties_.end());
  EXPECT_EQ(legacy_height->second.value_, "var(--main-height,120px)");
  EXPECT_EQ(legacy_height->second.text_, "height:var(--main-height,120px);");

  auto cached_legacy_sheet = legacy_variable_styles[0];
  auto cached_legacy_height =
      cached_legacy_sheet.css_properties_.find("height");
  ASSERT_NE(cached_legacy_height, cached_legacy_sheet.css_properties_.end());
  cached_legacy_height->second.value_ = "{{--main-height}}";
  cached_legacy_height->second.text_ = "height:{{--main-height}};";
  cached_legacy_sheet.css_text_ = "height:{{--main-height}};";
  cached_legacy_sheet.style_value_range_.end_column_ =
      cached_legacy_sheet.style_value_range_.start_column_ +
      static_cast<int>(cached_legacy_sheet.css_text_.size());
  lynx::devtool::ElementInspector::GetStyleSheetMap(style_root.get()).clear();
  lynx::devtool::ElementInspector::GetStyleSheetMap(style_root.get())
      .insert({"view", cached_legacy_sheet});

  auto cached_legacy_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(
          styled_element.get());
  ASSERT_EQ(cached_legacy_styles.size(), 1U);
  auto normalized_height =
      cached_legacy_styles[0].css_properties_.find("height");
  ASSERT_NE(normalized_height, cached_legacy_styles[0].css_properties_.end());
  EXPECT_EQ(normalized_height->second.value_, "var(--main-height,120px)");
  EXPECT_EQ(normalized_height->second.text_,
            "height:var(--main-height,120px);");
  EXPECT_EQ(cached_legacy_styles[0].css_text_,
            "height:var(--main-height,120px);");
  EXPECT_EQ(normalized_height->second.property_range_.end_column_,
            cached_legacy_styles[0].style_value_range_.end_column_);

  auto duplicate_legacy_sheet = cached_legacy_sheet;
  auto winning_height =
      duplicate_legacy_sheet.css_properties_.find("height")->second;
  auto earlier_height = winning_height;
  earlier_height.value_ = "{{--earlier-height}}";
  earlier_height.text_ = "height:{{--earlier-height}};";
  duplicate_legacy_sheet.css_properties_.clear();
  duplicate_legacy_sheet.css_properties_.emplace("height",
                                                 std::move(earlier_height));
  duplicate_legacy_sheet.css_properties_.emplace("height",
                                                 std::move(winning_height));
  duplicate_legacy_sheet.property_order_ = {"height", "height"};
  duplicate_legacy_sheet.css_text_ =
      "height:{{--earlier-height}};height:{{--main-height}};";
  duplicate_legacy_sheet.style_value_range_.end_column_ =
      duplicate_legacy_sheet.style_value_range_.start_column_ +
      static_cast<int>(duplicate_legacy_sheet.css_text_.size());
  lynx::devtool::ElementInspector::GetStyleSheetMap(style_root.get()).clear();
  lynx::devtool::ElementInspector::GetStyleSheetMap(style_root.get())
      .insert({"view", duplicate_legacy_sheet});

  auto duplicate_legacy_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(
          styled_element.get());
  ASSERT_EQ(duplicate_legacy_styles.size(), 1U);
  auto duplicate_height_range =
      duplicate_legacy_styles[0].css_properties_.equal_range("height");
  size_t earlier_declaration_count = 0;
  size_t winning_declaration_count = 0;
  for (auto it = duplicate_height_range.first;
       it != duplicate_height_range.second; ++it) {
    if (it->second.value_ == "{{--earlier-height}}") {
      ++earlier_declaration_count;
    } else if (it->second.value_ == "var(--main-height,120px)") {
      ++winning_declaration_count;
    }
  }
  EXPECT_EQ(earlier_declaration_count, 1U);
  EXPECT_EQ(winning_declaration_count, 1U);
  EXPECT_EQ(duplicate_legacy_styles[0].css_text_,
            "height:{{--earlier-height}};"
            "height:var(--main-height,120px);");

  lynx::tasm::CSSValue legacy_padding("{{--padding-y}} {{--padding-x}}",
                                      lynx::tasm::CSSValuePattern::STRING,
                                      lynx::tasm::CSSValueType::VARIABLE);
  lynx::lepus::Value padding_default_map(lynx::lepus::Dictionary::Create());
  padding_default_map.Table()->SetValue("--padding-y",
                                        lynx::lepus::Value("8px"));
  padding_default_map.Table()->SetValue("--padding-x",
                                        lynx::lepus::Value("12px"));
  legacy_padding.SetDefaultValueMap(std::move(padding_default_map));
  ASSERT_TRUE(legacy_padding.ToVarReference());
  variable_attributes.clear();
  variable_attributes.insert_or_assign(
      lynx::tasm::CSSPropertyID::kPropertyIDPadding, std::move(legacy_padding));
  variable_token->SetAttributes(std::move(variable_attributes));
  lynx::devtool::ElementInspector::GetStyleSheetMap(style_root.get()).clear();

  auto legacy_padding_styles =
      lynx::devtool::ElementInspector::GetMatchedStyleSheet(
          styled_element.get());
  ASSERT_EQ(legacy_padding_styles.size(), 1U);
  auto legacy_padding_property =
      legacy_padding_styles[0].css_properties_.find("padding");
  ASSERT_NE(legacy_padding_property,
            legacy_padding_styles[0].css_properties_.end());
  EXPECT_EQ(legacy_padding_property->second.value_,
            "var(--padding-y,8px) var(--padding-x,12px)");
}

TEST_F(ElementHelperTest,
       SetInlineStyleTextsNewPipelineSyncsImportantDeclarations) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  auto element = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  page->InsertNode(element);

  // Set inline style with mixed !important and normal declarations.
  lynx::devtool::ElementHelper::SetInlineStyleTexts(
      element.get(), "width:100px !important;height:200px;", Range(),
      page.get());

  // The inspector sheet still contains the original text (legacy parity).
  EXPECT_EQ(lynx::devtool::ElementInspector::GetInlineStyleSheet(element.get())
                .css_text_,
            "width:100px !important;height:200px;");

  // ProcessFullRawInlineStyle drains full_raw_inline_style_ into the current
  // inline maps. Important declarations should be preserved in the important
  // map instead of being dropped by the devtool bridge.
  EXPECT_TRUE(element->GetRawInlineStyles().str().empty());
  const auto& inline_styles = element->GetCurrentRawInlineStyles();
  ASSERT_TRUE(inline_styles.has_value());
  EXPECT_EQ(inline_styles->count(lynx::tasm::CSSPropertyID::kPropertyIDWidth),
            0U);
  auto height_it =
      inline_styles->find(lynx::tasm::CSSPropertyID::kPropertyIDHeight);
  ASSERT_TRUE(height_it != inline_styles->end());
  EXPECT_EQ(height_it->second.StdString(), "200px");

  const auto& important_inline_styles =
      element->GetCurrentRawImportantInlineStyles();
  ASSERT_TRUE(important_inline_styles.has_value());
  auto width_it = important_inline_styles->find(
      lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != important_inline_styles->end());
  EXPECT_EQ(width_it->second.StdString(), "100px");

  lynx::devtool::ElementHelper::SetInlineStyleTexts(
      element.get(), "width:150px;", Range(), page.get());

  const auto& updated_inline_styles = element->GetCurrentRawInlineStyles();
  ASSERT_TRUE(updated_inline_styles.has_value());
  auto updated_width_it =
      updated_inline_styles->find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(updated_width_it != updated_inline_styles->end());
  EXPECT_EQ(updated_width_it->second.StdString(), "150px");
  const auto& updated_important_inline_styles =
      element->GetCurrentRawImportantInlineStyles();
  EXPECT_TRUE(!updated_important_inline_styles.has_value() ||
              updated_important_inline_styles->find(
                  lynx::tasm::CSSPropertyID::kPropertyIDWidth) ==
                  updated_important_inline_styles->end());
}

TEST_F(ElementHelperTest,
       SetInlineStyleTextsNewPipelinePreservesImportantCustomProperty) {
  manager->enable_new_styling_pipeline_ = true;
  lynx::base::AutoReset<bool> css_inline_config(
      &(manager->GetConfig()->css_configs_.enable_css_inline_variables_), true);

  auto page = manager->CreateFiberPage("page", 0);
  auto comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp->attribute_holder());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  page->InsertNode(element);

  lynx::devtool::ElementHelper::SetInlineStyleTexts(
      element.get(), "--theme:red !important;color:var(--theme);", Range(),
      page.get());

  // Inspector retains the original text.
  EXPECT_EQ(lynx::devtool::ElementInspector::GetInlineStyleSheet(element.get())
                .css_text_,
            "--theme:red !important;color:var(--theme);");

  // Element inline styles keep the var() user, while the custom property is
  // preserved without its !important suffix.
  EXPECT_TRUE(element->GetRawInlineStyles().str().empty());
  const auto& inline_styles = element->GetCurrentRawInlineStyles();
  ASSERT_TRUE(inline_styles.has_value());
  EXPECT_EQ(inline_styles->count(lynx::tasm::CSSPropertyID::kPropertyIDColor),
            1U);

  auto inline_vars = element->data_model()->GetCSSInlineVariables();
  auto theme_it = inline_vars.find(lynx::base::String("--theme"));
  ASSERT_TRUE(theme_it != inline_vars.end());
  EXPECT_EQ(theme_it->second.str(), "red");
}

TEST_F(ElementHelperTest,
       SetSelectorStyleTextsNewPipelineSyncsImportantDeclarations) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  auto selector_fragment = CreateSelectorCSSFragment(
      "view", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px");
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  lynx::base::String component_id("21");
  int32_t css_id = 100;
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto styled_element = manager->CreateFiberElement("view");
  auto styled_comp = std::shared_ptr<lynx::tasm::RadonComponent>(
      new lynx::tasm::RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  styled_element->SetAttributeHolder(styled_comp->attribute_holder());
  styled_element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  component->InsertNode(styled_element);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(styled_element.get()));

  auto style_root = manager->CreateFiberElement("stylevalue");
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(style_root.get(), style_root.get()));
  lynx::devtool::ElementInspector::SetStyleRoot(
      std::make_tuple(styled_element.get(), style_root.get()));

  auto initial_sheet = lynx::devtool::ElementInspector::GetStyleSheetByName(
      styled_element.get(), "view");
  ASSERT_FALSE(initial_sheet.empty);

  auto token =
      styled_element->GetRelatedCSSFragment()->GetSharedCSSStyle("view");
  ASSERT_NE(token, nullptr);
  lynx::tasm::StyleMap stale_important_styles;
  stale_important_styles.insert_or_assign(
      lynx::tasm::CSSPropertyID::kPropertyIDWidth,
      lynx::tasm::CSSValue(10, lynx::tasm::CSSValuePattern::PX));
  token->SetImportantAttributes(std::move(stale_important_styles));

  // Edit selector style to move width out of !important and add height as a
  // new important declaration. The old important width must not keep
  // overriding, and important custom properties should be preserved without
  // their suffix.
  lynx::devtool::ElementHelper::SetStyleTexts(
      page.get(), style_root.get(),
      "--theme:red !important;color:var(--theme);width:20px;"
      "height:30px !important;",
      initial_sheet.style_value_range_);

  const auto& attributes = token->GetAttributes();
  auto color_it = attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDColor);
  ASSERT_TRUE(color_it != attributes.end());
  auto width_it = attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != attributes.end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDWidth, width_it->second),
            "20px");

  const auto& important_attributes = token->GetImportantAttributes();
  EXPECT_TRUE(
      important_attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth) ==
      important_attributes.end());
  auto height_it =
      important_attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDHeight);
  ASSERT_TRUE(height_it != important_attributes.end());
  EXPECT_EQ(
      lynx::tasm::CSSDecoder::CSSValueToString(
          lynx::tasm::CSSPropertyID::kPropertyIDHeight, height_it->second),
      "30px");

  const auto& style_variables = token->GetStyleVariables();
  auto theme_it = style_variables.find(lynx::base::String("--theme"));
  ASSERT_TRUE(theme_it != style_variables.end());
  EXPECT_EQ(theme_it->second.str(), "red");
}

TEST_F(ElementHelperTest, SetAttributesClassNewPipeline) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  auto selector_fragment = CreateSelectorCSSFragment(
      ".foo", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px");
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  lynx::base::String component_id("21");
  int32_t css_id = 100;
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto element = manager->CreateFiberElement("view");
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  component->InsertNode(element);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  // Set class via devtool.
  lynx::devtool::ElementHelper::SetAttributes(element.get(), "class", "foo");

  // AttributeHolder should have the class.
  ASSERT_EQ(element->data_model()->classes().size(), 1U);
  EXPECT_EQ(element->data_model()->classes()[0].str(), "foo");

  // Computed style should reflect the .foo rule.
  auto resolved = element->computed_css_style()->GetResolvedValues();
  auto width_it = resolved.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != resolved.end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDWidth, width_it->second),
            "10px");

  // Remove class via devtool.
  lynx::devtool::ElementHelper::RemoveAttributes(element.get(), "class");

  EXPECT_TRUE(element->data_model()->classes().empty());

  // After removal, width should fall back to default (not present in
  // resolved).
  auto resolved_after = element->computed_css_style()->GetResolvedValues();
  EXPECT_TRUE(
      resolved_after.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth) ==
      resolved_after.end());
}

TEST_F(ElementHelperTest, SetAttributesIdNewPipeline) {
  manager->enable_new_styling_pipeline_ = true;

  auto page = manager->CreateFiberPage("page", 0);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(page.get()));
  auto selector_fragment = CreateSelectorCSSFragment(
      "#bar", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "20px");
  page->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());

  lynx::base::String component_id("21");
  int32_t css_id = 100;
  lynx::base::String entry_name("__Card__");
  lynx::base::String component_name("TestComp");
  lynx::base::String path("/index/components/TestComp");
  auto component = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  component->style_sheet_ = std::make_unique<lynx::tasm::CSSFragmentDecorator>(
      selector_fragment.get());
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(component.get()));
  page->InsertNode(component);

  auto element = manager->CreateFiberElement("view");
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(component->impl_id()));
  component->InsertNode(element);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));

  // Set id via devtool.
  lynx::devtool::ElementHelper::SetAttributes(element.get(), "id", "bar");

  EXPECT_EQ(element->data_model()->idSelector().str(), "bar");

  auto resolved = element->computed_css_style()->GetResolvedValues();
  auto width_it = resolved.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != resolved.end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDWidth, width_it->second),
            "20px");

  // Remove id via devtool.
  lynx::devtool::ElementHelper::RemoveAttributes(element.get(), "id");

  EXPECT_TRUE(element->data_model()->idSelector().empty());

  auto resolved_after = element->computed_css_style()->GetResolvedValues();
  EXPECT_TRUE(
      resolved_after.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth) ==
      resolved_after.end());
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
