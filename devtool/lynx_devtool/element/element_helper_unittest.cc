// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/element/element_helper.h"

#include "base/include/auto_reset.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
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

std::shared_ptr<lynx::tasm::SharedCSSFragment> CreateSelectorCSSFragment(
    const std::string& selector, lynx::tasm::CSSPropertyID property_id,
    const std::string& value) {
  lynx::tasm::CSSParserConfigs parser_configs;
  auto token = fml::MakeRefCounted<lynx::tasm::CSSParseToken>(parser_configs);
  token->raw_attributes_[property_id] =
      lynx::tasm::CSSValue::MakePlainString(value.c_str());

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
  fragment->AddStyleRule(std::move(selector_arr), token);

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
  auto selector_fragment = CreateSelectorCSSFragment(
      ".card .title", lynx::tasm::CSSPropertyID::kPropertyIDWidth, "10px");
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

  lynx::devtool::ElementHelper::SetStyleTexts(page.get(), style_root.get(),
                                              "width:20px;",
                                              initial_sheet.style_value_range_);

  auto token = styled_element->GetRelatedCSSFragment()->GetSharedCSSStyle(
      ".card .title");
  ASSERT_NE(token, nullptr);

  const auto& attributes = token->GetAttributes();
  auto width_it = attributes.find(lynx::tasm::CSSPropertyID::kPropertyIDWidth);
  ASSERT_TRUE(width_it != attributes.end());
  EXPECT_EQ(lynx::tasm::CSSDecoder::CSSValueToString(
                lynx::tasm::CSSPropertyID::kPropertyIDWidth, width_it->second),
            "20px");
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
  adopted_fragment->AddStyleRule(std::move(selector_arr), adopted_token);

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
}

TEST_F(ElementHelperTest, InitStyleSheetTest) {
  auto element = manager->CreateFiberElement("view");
  lynx::devtool::ElementInspector::InitForInspector(std::make_tuple(element));

  std::unordered_map<std::string, std::string> styles = {
      {"font-size", "32rpx"}};
  lynx::devtool::InspectorStyleSheet init_res =
      lynx::devtool::ElementInspector::InitStyleSheet(element.get(), 10,
                                                      "font-size", styles, 0);

  std::unordered_multimap<std::string, lynx::devtool::CSSPropertyDetail>
      css_properties = {{"font-size",
                         {"font-size",
                          "32rpx",
                          "font-size:32rpx;",
                          false,
                          false,
                          false,
                          false,
                          true,
                          {80, 10, 65, 10}}}};
  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.empty = false;
  style_sheet.style_sheet_id_ = std::to_string(element.get()->impl_id());
  style_sheet.style_name_ = "font-size";
  style_sheet.origin_ = "regular";
  style_sheet.css_text_ = "font-size:32rpx;";
  style_sheet.css_properties_ = css_properties;
  style_sheet.style_name_range_ = {10, 10, 0, 9};
  style_sheet.style_value_range_ = {10, 10, 10, 26};
  style_sheet.position_ = 0;

  EXPECT_EQ(init_res.empty, style_sheet.empty);
  EXPECT_EQ(init_res.style_sheet_id_, style_sheet.style_sheet_id_);
  EXPECT_EQ(init_res.style_name_, style_sheet.style_name_);
  EXPECT_EQ(init_res.origin_, style_sheet.origin_);
  EXPECT_EQ(init_res.css_text_, style_sheet.css_text_);
  EXPECT_EQ(init_res.css_properties_.size(),
            style_sheet.css_properties_.size());

  for (auto it : init_res.css_properties_) {
    EXPECT_EQ(it.first, style_sheet.css_properties_.find(it.first)->first);
    EXPECT_EQ(it.second.text_,
              style_sheet.css_properties_.find(it.first)->second.text_);
  }

  EXPECT_EQ(init_res.style_name_range_.start_column_,
            style_sheet.style_name_range_.start_column_);
  EXPECT_EQ(init_res.style_name_range_.end_line_,
            style_sheet.style_name_range_.end_line_);
  EXPECT_EQ(init_res.style_name_range_.end_column_,
            style_sheet.style_name_range_.end_column_);
  EXPECT_EQ(init_res.style_name_range_.start_line_,
            style_sheet.style_name_range_.start_line_);

  EXPECT_EQ(init_res.style_value_range_.start_line_,
            style_sheet.style_value_range_.start_line_);
  EXPECT_EQ(init_res.style_value_range_.start_column_,
            style_sheet.style_value_range_.start_column_);
  EXPECT_EQ(init_res.style_value_range_.end_column_,
            style_sheet.style_value_range_.end_column_);
  EXPECT_EQ(init_res.style_value_range_.end_line_,
            style_sheet.style_value_range_.end_line_);

  EXPECT_EQ(init_res.position_, style_sheet.position_);
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

  // Edit selector style to move width out of !important and add height as a new
  // important declaration. The old important width must not keep overriding,
  // and important custom properties should be preserved without their suffix.
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

  // After removal, width should fall back to default (not present in resolved).
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
