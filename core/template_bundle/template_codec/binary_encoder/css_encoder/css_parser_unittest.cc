// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"

#include "core/base/json/json_util.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser_token.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(CSSParserDiagnostics, CollectUnsupportedProperty) {
  CompileOptions options;
  CSSParser parser(options);

  std::string json_input = R"({
    "type": "StyleRule",
    "selectorText": {"value": ".container", "loc": {"line": 1, "column": 1}},
    "style": [
      {"name": "width", "value": "100px", "keyLoc": {"line": 2, "column": 3}},
      {"name": "unsupported-property-xyz", "value": "1px", "keyLoc": {"line": 3, "column": 5}}
    ],
    "variables": {}
  })";

  auto json = base::strToJson(json_input.c_str());
  parser.CollectStyleDiagnostics(json);

  const auto& diags = parser.css_diagnostics();
  ASSERT_EQ(diags.size(), 1);
  EXPECT_EQ(diags[0].type, "property");
  EXPECT_EQ(diags[0].name, "unsupported-property-xyz");
  EXPECT_EQ(diags[0].line, 3);
  EXPECT_EQ(diags[0].column, 5);
}

TEST(CSSParserDiagnostics, CollectUnsupportedPropertyWithoutLoc) {
  CompileOptions options;
  CSSParser parser(options);

  std::string json_input = R"({
    "type": "StyleRule",
    "selectorText": {"value": ".container"},
    "style": [
      {"name": "bad-prop", "value": "1px"}
    ],
    "variables": {}
  })";

  auto json = base::strToJson(json_input.c_str());
  parser.CollectStyleDiagnostics(json);

  const auto& diags = parser.css_diagnostics();
  ASSERT_EQ(diags.size(), 1);
  EXPECT_EQ(diags[0].type, "property");
  EXPECT_EQ(diags[0].name, "bad-prop");
  EXPECT_EQ(diags[0].line, -1);
  EXPECT_EQ(diags[0].column, -1);
}

TEST(CSSParserDiagnostics, LegacySelectorParseFailed) {
  CompileOptions options;
  CSSParser parser(options);

  std::string json_input = R"({
    "type": "StyleRule",
    "selectorText": {"value": ":not", "loc": {"line": 5, "column": 10}},
    "style": [],
    "variables": {}
  })";

  auto json = base::strToJson(json_input.c_str());
  CSSParserTokenMap css;
  parser.ParseCSSTokens(css, json, "/test.css");

  const auto& diags = parser.css_diagnostics();
  ASSERT_EQ(diags.size(), 1);
  EXPECT_EQ(diags[0].type, "selector");
  EXPECT_EQ(diags[0].name, ":not");
  EXPECT_EQ(diags[0].line, 5);
  EXPECT_EQ(diags[0].column, 10);
}

TEST(CSSParserDiagnostics, NGSelectorParseFailed) {
  CompileOptions options;
  options.enable_css_selector_ = true;
  CSSParser parser(options);

  std::string json_input = R"({
    "type": "StyleRule",
    "selectorText": {"value": ":::", "loc": {"line": 7, "column": 14}},
    "style": [],
    "variables": {}
  })";

  auto json = base::strToJson(json_input.c_str());
  CSSParserTokenMap css;
  std::vector<encoder::LynxCSSSelectorTuple> tuples;
  parser.ParseCSSTokensNew(tuples, css, json, "/test.css");

  const auto& diags = parser.css_diagnostics();
  ASSERT_EQ(diags.size(), 1);
  EXPECT_EQ(diags[0].type, "selector");
  EXPECT_EQ(diags[0].name, ":::");
  EXPECT_EQ(diags[0].line, 7);
  EXPECT_EQ(diags[0].column, 14);
}

TEST(CSSParserDiagnostics, NoDiagnosticsForValidRule) {
  CompileOptions options;
  options.enable_css_selector_ = false;
  CSSParser parser(options);

  std::string json_input = R"({
    "type": "StyleRule",
    "selectorText": {"value": ".container", "loc": {"line": 1, "column": 1}},
    "style": [
      {"name": "width", "value": "100px", "keyLoc": {"line": 2, "column": 3}}
    ],
    "variables": {}
  })";

  auto json = base::strToJson(json_input.c_str());
  CSSParserTokenMap css;
  parser.ParseCSSTokens(css, json, "/test.css");

  EXPECT_TRUE(parser.css_diagnostics().empty());
}

TEST(CSSParserDiagnostics, GetCSSDiagnosticsJson) {
  CompileOptions options;
  CSSParser parser(options);

  std::string json_input = R"({
    "type": "StyleRule",
    "selectorText": {"value": ".container", "loc": {"line": 1, "column": 1}},
    "style": [
      {"name": "bad-prop", "value": "1px", "keyLoc": {"line": 2, "column": 3}}
    ],
    "variables": {}
  })";

  auto json = base::strToJson(json_input.c_str());
  parser.CollectStyleDiagnostics(json);
  parser.CollectSelectorDiagnostics(json, ".container");

  std::string result = parser.GetCSSDiagnosticsJson();
  EXPECT_NE(result.find("\"type\":\"property\""), std::string::npos);
  EXPECT_NE(result.find("\"name\":\"bad-prop\""), std::string::npos);
  EXPECT_NE(result.find("\"line\":2"), std::string::npos);
  EXPECT_NE(result.find("\"column\":3"), std::string::npos);
  EXPECT_NE(result.find("\"type\":\"selector\""), std::string::npos);
  EXPECT_NE(result.find("\"name\":\".container\""), std::string::npos);
  EXPECT_NE(result.find("\"line\":1"), std::string::npos);
  EXPECT_NE(result.find("\"column\":1"), std::string::npos);
}

TEST(CSSParser, MergeCSSParseTokenPreservesImportantAttributes) {
  rapidjson::Document doc;
  doc.SetObject();

  rapidjson::Value style_origin(rapidjson::kArrayType);
  rapidjson::Value entry_origin(rapidjson::kObjectType);
  entry_origin.AddMember("name", "width", doc.GetAllocator());
  entry_origin.AddMember("value", "100px !important", doc.GetAllocator());
  style_origin.PushBack(entry_origin, doc.GetAllocator());

  rapidjson::Value style_new(rapidjson::kArrayType);
  rapidjson::Value entry_new(rapidjson::kObjectType);
  entry_new.AddMember("name", "width", doc.GetAllocator());
  entry_new.AddMember("value", "200px", doc.GetAllocator());
  style_new.PushBack(entry_new, doc.GetAllocator());

  rapidjson::Value style_vars(rapidjson::kObjectType);
  std::string rule = ".container";
  tasm::CompileOptions options;
  options.enable_css_parser_ = true;
  options.target_sdk_version_ = "3.9";

  fml::RefPtr<tasm::CSSParseToken> origin =
      fml::MakeRefCounted<encoder::CSSParseToken>(style_origin, rule, "",
                                                  style_vars, options);
  fml::RefPtr<tasm::CSSParseToken> new_token =
      fml::MakeRefCounted<encoder::CSSParseToken>(style_new, rule, "",
                                                  style_vars, options);

  EXPECT_TRUE(
      origin->GetImportantAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_FALSE(origin->GetAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_TRUE(new_token->GetAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_FALSE(
      new_token->GetImportantAttributes().contains(tasm::kPropertyIDWidth));

  CSSParser::MergeCSSParseToken(origin, new_token);

  EXPECT_TRUE(origin->GetAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_TRUE(
      origin->GetImportantAttributes().contains(tasm::kPropertyIDWidth));
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
