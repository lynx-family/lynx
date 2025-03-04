// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/element_helper.h"

#include "core/base/threading/task_runner_manufactor.h"
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

  auto element = manager->CreateNode("view", nullptr);

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
  auto element1 = manager->CreateNode("view", nullptr);
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

  auto element = manager->CreateNode("view", nullptr);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element.get()));
  element->CreateElementContainer(false);
  auto element_container = element->element_container();

  auto child = manager->CreateNode("view", nullptr);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child.get()));
  element->AddChildAt(child.get(), 0);
  EXPECT_EQ(child->parent(), element.get());

  child->CreateElementContainer(false);
  auto child_container = child->element_container();
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

TEST_F(ElementHelperTest, GetElementContentTest) {
  auto element = manager->CreateNode("view", nullptr);
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
  auto element = manager->CreateNode("view", nullptr);
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
  auto element1 = manager->CreateNode("view", nullptr);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(element1.get()));
  element1->CreateElementContainer(false);

  auto child1 = manager->CreateNode("view", nullptr);
  lynx::devtool::ElementInspector::InitForInspector(
      std::make_tuple(child1.get()));
  element1->AddChildAt(child1.get(), 0);

  child1->CreateElementContainer(false);
  auto child_container = child1->element_container();
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
  auto element = manager->CreateNode("view", comp.get()->attribute_holder());
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
  auto element = manager->CreateNode("view", comp.get()->attribute_holder());
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
  auto element = manager->CreateNode("view", comp.get()->attribute_holder());
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

TEST_F(ElementHelperTest, InitStyleSheetTest) {
  auto element = manager->CreateNode("view", nullptr);
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

  auto element = manager->CreateNode("view", comp.get()->attribute_holder());
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
  auto element = manager->CreateNode("view", nullptr);
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
  auto element = manager->CreateNode("view", comp.get()->attribute_holder());
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

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
