// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"

#include "core/base/json/json_util.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_rule_parser.h"
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

TEST(CSSParserDiagnostics, ParseExternalFragmentCollectsDiagnostics) {
  CompileOptions options;
  CSSParser parser(options);

  std::string rule_list_json = R"([{
    "type": "StyleRule",
    "selectorText": {"value": ".custom", "loc": {"line": 1, "column": 1}},
    "style": [
      {"name": "unknown-custom-prop", "value": "1px", "keyLoc": {"line": 2, "column": 3}}
    ],
    "variables": {}
  }])";

  rapidjson::Document doc;
  doc.Parse(rule_list_json.c_str());
  ASSERT_FALSE(doc.HasParseError());

  auto fragment = parser.ParseExternalFragment(doc, "/custom.css");
  ASSERT_NE(fragment, nullptr);

  const auto& diags = parser.css_diagnostics();
  ASSERT_EQ(diags.size(), 1);
  EXPECT_EQ(diags[0].type, "property");
  EXPECT_EQ(diags[0].name, "unknown-custom-prop");
  EXPECT_EQ(diags[0].line, 2);
  EXPECT_EQ(diags[0].column, 3);
}

class CSSRuleParserTest : public ::testing::Test {
 protected:
  CompileOptions compile_options_;
};

TEST_F(CSSRuleParserTest, ParseSingleStyleRule) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        {
          "name": "font-size",
          "value": "30px",
          "keyLoc": { "line": 1, "column": 16 },
          "valLoc": { "line": 1, "column": 22 }
        }
      ],
      "selectorText": {
        "value": "#text",
        "loc": { "line": 1, "column": 6 }
      },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kStyle);

  auto* style_rule = static_cast<encoder::LynxStyleRule*>(rules[0].get());
  EXPECT_GT(style_rule->flattened_size, 0u);
  EXPECT_EQ(style_rule->position, 0u);
  EXPECT_NE(style_rule->selector_arr, nullptr);
  EXPECT_NE(style_rule->properties, nullptr);
}

TEST_F(CSSRuleParserTest, ParseMediaRule) {
  const char* json_str = R"json([
    {
      "type": "MediaRule",
      "prelude": {
        "value": "(max-width:1250px)",
        "loc": { "line": 1, "column": 47 }
      },
      "rules": [
        {
          "type": "StyleRule",
          "style": [
            {
              "name": "font-size",
              "value": "50px",
              "keyLoc": { "line": 1, "column": 63 },
              "valLoc": { "line": 1, "column": 69 }
            }
          ],
          "selectorText": {
            "value": "#text",
            "loc": { "line": 1, "column": 53 }
          },
          "variables": {}
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kMedia);

  auto* media_rule =
      static_cast<encoder::LynxStyleRuleCondition*>(rules[0].get());
  EXPECT_EQ(media_rule->condition, "(max-width:1250px)");
  ASSERT_EQ(media_rule->child_rules.size(), 1u);
  EXPECT_EQ(media_rule->child_rules[0]->type, CSSRuleType::kStyle);

  auto* child_style =
      static_cast<encoder::LynxStyleRule*>(media_rule->child_rules[0].get());
  EXPECT_GT(child_style->flattened_size, 0u);
  EXPECT_NE(child_style->properties, nullptr);
}

TEST_F(CSSRuleParserTest, ParseSupportsRule) {
  const char* json_str = R"json([
    {
      "type": "SupportsRule",
      "prelude": {
        "value": "(display:flex)",
        "loc": { "line": 1, "column": 94 }
      },
      "rules": []
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kSupports);

  auto* supports_rule =
      static_cast<encoder::LynxStyleRuleCondition*>(rules[0].get());
  EXPECT_EQ(supports_rule->condition, "(display:flex)");
  EXPECT_EQ(supports_rule->child_rules.size(), 0u);
}

TEST_F(CSSRuleParserTest, ParseFontFaceRule) {
  const char* json_str = R"json([
    {
      "type": "FontFaceRule",
      "style": [
        {
          "name": "font-family",
          "value": "Bitstream Vera Serif Bold",
          "keyLoc": { "line": 1, "column": 118 },
          "valLoc": { "line": 1, "column": 147 }
        },
        {
          "name": "src",
          "value": "url(https://example.com/font.woff2)",
          "keyLoc": { "line": 1, "column": 150 },
          "valLoc": { "line": 1, "column": 221 }
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kFontFace);

  auto* fontface_rule =
      static_cast<encoder::LynxStyleRuleFontFace*>(rules[0].get());
  EXPECT_EQ(fontface_rule->family, "Bitstream Vera Serif Bold");
  EXPECT_EQ(fontface_rule->properties.size(), 1u);
}

TEST_F(CSSRuleParserTest, ParseKeyframesRule) {
  const char* json_str = R"json([
    {
      "type": "KeyframesRule",
      "name": {
        "value": "ani",
        "loc": { "line": 1, "column": 235 }
      },
      "styles": [
        {
          "keyText": {
            "value": "0%",
            "loc": { "line": 1, "column": 238 }
          },
          "variables": {},
          "style": [
            {
              "name": "background-color",
              "value": "blue",
              "keyLoc": { "line": 1, "column": 255 },
              "valLoc": { "line": 1, "column": 261 }
            }
          ]
        },
        {
          "keyText": {
            "value": "100%",
            "loc": { "line": 1, "column": 275 }
          },
          "variables": {},
          "style": [
            {
              "name": "opacity",
              "value": "1",
              "keyLoc": { "line": 1, "column": 283 },
              "valLoc": { "line": 1, "column": 286 }
            }
          ]
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kKeyframes);

  auto* keyframes_rule =
      static_cast<encoder::LynxStyleRuleKeyframes*>(rules[0].get());
  EXPECT_EQ(keyframes_rule->name, "ani");
  EXPECT_NE(keyframes_rule->properties, nullptr);
}

TEST_F(CSSRuleParserTest, ParseAllRuleTypes) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "font-size", "value": "30px",
          "keyLoc": { "line": 1, "column": 16 },
          "valLoc": { "line": 1, "column": 22 } }
      ],
      "selectorText": { "value": "#text", "loc": { "line": 1, "column": 6 } },
      "variables": {}
    },
    {
      "type": "MediaRule",
      "prelude": { "value": "(max-width:1250px)", "loc": { "line": 1, "column": 47 } },
      "rules": [
        {
          "type": "StyleRule",
          "style": [
            { "name": "font-size", "value": "50px",
              "keyLoc": { "line": 1, "column": 63 },
              "valLoc": { "line": 1, "column": 69 } }
          ],
          "selectorText": { "value": "#text", "loc": { "line": 1, "column": 53 } },
          "variables": {}
        }
      ]
    },
    {
      "type": "SupportsRule",
      "prelude": { "value": "(display:flex)", "loc": { "line": 1, "column": 94 } },
      "rules": []
    },
    {
      "type": "FontFaceRule",
      "style": [
        { "name": "font-family", "value": "Bitstream Vera Serif Bold",
          "keyLoc": { "line": 1, "column": 118 },
          "valLoc": { "line": 1, "column": 147 } },
        { "name": "src", "value": "url(https://example.com/font.woff2)",
          "keyLoc": { "line": 1, "column": 150 },
          "valLoc": { "line": 1, "column": 221 } }
      ]
    },
    {
      "type": "KeyframesRule",
      "name": { "value": "ani", "loc": { "line": 1, "column": 235 } },
      "styles": [
        {
          "keyText": { "value": "0%", "loc": { "line": 1, "column": 238 } },
          "variables": {},
          "style": [
            { "name": "background-color", "value": "blue",
              "keyLoc": { "line": 1, "column": 255 },
              "valLoc": { "line": 1, "column": 261 } }
          ]
        },
        {
          "keyText": { "value": "100%", "loc": { "line": 1, "column": 275 } },
          "variables": {},
          "style": [
            { "name": "opacity", "value": "1",
              "keyLoc": { "line": 1, "column": 283 },
              "valLoc": { "line": 1, "column": 286 } }
          ]
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 5u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kStyle);
  EXPECT_EQ(rules[1]->type, CSSRuleType::kMedia);
  EXPECT_EQ(rules[2]->type, CSSRuleType::kSupports);
  EXPECT_EQ(rules[3]->type, CSSRuleType::kFontFace);
  EXPECT_EQ(rules[4]->type, CSSRuleType::kKeyframes);
}

TEST_F(CSSRuleParserTest, ParseEmptyRules) {
  const char* json_str = "[]";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  EXPECT_EQ(fragment->rules().size(), 0u);
}

TEST_F(CSSRuleParserTest, ParseRuleWithoutType) {
  const char* json_str = R"json([
    {
      "style": [
        { "name": "font-size", "value": "30px" }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  EXPECT_EQ(fragment->rules().size(), 0u);
}

TEST_F(CSSRuleParserTest, ParseStyleRuleWithClassSelector) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "red",
          "keyLoc": { "line": 2, "column": 3 },
          "valLoc": { "line": 2, "column": 10 } }
      ],
      "selectorText": {
        "value": ".container",
        "loc": { "line": 1, "column": 1 }
      },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);

  auto* style_rule = static_cast<encoder::LynxStyleRule*>(rules[0].get());
  EXPECT_EQ(style_rule->position, 0u);
  EXPECT_GT(style_rule->flattened_size, 0u);
}

TEST_F(CSSRuleParserTest, ParseMediaRuleWithMultipleChildRules) {
  const char* json_str = R"json([
    {
      "type": "MediaRule",
      "prelude": { "value": "(min-width:600px)", "loc": { "line": 1, "column": 1 } },
      "rules": [
        {
          "type": "StyleRule",
          "style": [
            { "name": "width", "value": "100%",
              "keyLoc": { "line": 2, "column": 3 },
              "valLoc": { "line": 2, "column": 10 } }
          ],
          "selectorText": { "value": ".a", "loc": { "line": 2, "column": 1 } },
          "variables": {}
        },
        {
          "type": "StyleRule",
          "style": [
            { "name": "height", "value": "50%",
              "keyLoc": { "line": 3, "column": 3 },
              "valLoc": { "line": 3, "column": 11 } }
          ],
          "selectorText": { "value": ".b", "loc": { "line": 3, "column": 1 } },
          "variables": {}
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);

  auto* media_rule =
      static_cast<encoder::LynxStyleRuleCondition*>(rules[0].get());
  EXPECT_EQ(media_rule->condition, "(min-width:600px)");
  ASSERT_EQ(media_rule->child_rules.size(), 2u);
  EXPECT_EQ(media_rule->child_rules[0]->type, CSSRuleType::kStyle);
  EXPECT_EQ(media_rule->child_rules[1]->type, CSSRuleType::kStyle);
}

TEST_F(CSSRuleParserTest, ParseFragmentIdAndDependentList) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "blue",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 8 } }
      ],
      "selectorText": { "value": ".x", "loc": { "line": 1, "column": 1 } },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  std::vector<int32_t> deps = {1, 2, 3};
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", deps, 42);

  ASSERT_NE(fragment, nullptr);
  EXPECT_EQ(fragment->id(), 42);
  EXPECT_EQ(fragment->dependent_ids().size(), 3u);
}

TEST_F(CSSRuleParserTest, ParseUnknownRuleTypeIgnored) {
  const char* json_str = R"json([
    {
      "type": "UnknownRule",
      "data": "something"
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  EXPECT_EQ(fragment->rules().size(), 0u);
}

TEST_F(CSSRuleParserTest, ParseStyleRuleWithoutStyleMember) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "selectorText": { "value": "#text", "loc": { "line": 1, "column": 1 } },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  EXPECT_EQ(fragment->rules().size(), 0u);
}

TEST_F(CSSRuleParserTest, ParseStyleRuleWithoutSelectorText) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "red",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 8 } }
      ],
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  EXPECT_EQ(fragment->rules().size(), 0u);
}

TEST_F(CSSRuleParserTest, ParseStyleRulePositionIncrementsAcrossMultipleRules) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "red",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 8 } }
      ],
      "selectorText": { "value": ".a", "loc": { "line": 1, "column": 1 } },
      "variables": {}
    },
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "blue",
          "keyLoc": { "line": 2, "column": 1 },
          "valLoc": { "line": 2, "column": 8 } }
      ],
      "selectorText": { "value": ".b", "loc": { "line": 2, "column": 1 } },
      "variables": {}
    },
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "green",
          "keyLoc": { "line": 3, "column": 1 },
          "valLoc": { "line": 3, "column": 8 } }
      ],
      "selectorText": { "value": ".c", "loc": { "line": 3, "column": 1 } },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 3u);

  auto* rule0 = static_cast<encoder::LynxStyleRule*>(rules[0].get());
  auto* rule1 = static_cast<encoder::LynxStyleRule*>(rules[1].get());
  auto* rule2 = static_cast<encoder::LynxStyleRule*>(rules[2].get());
  EXPECT_EQ(rule0->position, 0u);
  EXPECT_EQ(rule1->position, 1u);
  EXPECT_EQ(rule2->position, 2u);
}

TEST_F(CSSRuleParserTest, ParseCSSRulesResetsCounterOnSecondCall) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "red",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 8 } }
      ],
      "selectorText": { "value": ".a", "loc": { "line": 1, "column": 1 } },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment1 = impl.ParseCSSRules(json, "/test1.ttss", {}, 1);
  ASSERT_NE(fragment1, nullptr);
  ASSERT_EQ(fragment1->rules().size(), 1u);
  auto* rule1 =
      static_cast<encoder::LynxStyleRule*>(fragment1->rules()[0].get());
  EXPECT_EQ(rule1->position, 0u);

  auto fragment2 = impl.ParseCSSRules(json, "/test2.ttss", {}, 2);
  ASSERT_NE(fragment2, nullptr);
  ASSERT_EQ(fragment2->rules().size(), 1u);
  auto* rule2 =
      static_cast<encoder::LynxStyleRule*>(fragment2->rules()[0].get());
  EXPECT_EQ(rule2->position, 0u);
}

TEST_F(CSSRuleParserTest, ParseMediaRuleWithoutPrelude) {
  const char* json_str = R"json([
    {
      "type": "MediaRule",
      "rules": [
        {
          "type": "StyleRule",
          "style": [
            { "name": "color", "value": "red",
              "keyLoc": { "line": 1, "column": 1 },
              "valLoc": { "line": 1, "column": 8 } }
          ],
          "selectorText": { "value": ".a", "loc": { "line": 1, "column": 1 } },
          "variables": {}
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kMedia);

  auto* media_rule =
      static_cast<encoder::LynxStyleRuleCondition*>(rules[0].get());
  EXPECT_EQ(media_rule->condition, "");
  ASSERT_EQ(media_rule->child_rules.size(), 1u);
}

TEST_F(CSSRuleParserTest, ParseMediaRuleWithoutRulesField) {
  const char* json_str = R"json([
    {
      "type": "MediaRule",
      "prelude": { "value": "(min-width:600px)", "loc": { "line": 1, "column": 1 } }
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);

  auto* media_rule =
      static_cast<encoder::LynxStyleRuleCondition*>(rules[0].get());
  EXPECT_EQ(media_rule->condition, "(min-width:600px)");
  EXPECT_EQ(media_rule->child_rules.size(), 0u);
}

TEST_F(CSSRuleParserTest, ParseSupportsRuleWithChildRules) {
  const char* json_str = R"json([
    {
      "type": "SupportsRule",
      "prelude": { "value": "(display:grid)", "loc": { "line": 1, "column": 1 } },
      "rules": [
        {
          "type": "StyleRule",
          "style": [
            { "name": "display", "value": "grid",
              "keyLoc": { "line": 2, "column": 3 },
              "valLoc": { "line": 2, "column": 12 } }
          ],
          "selectorText": { "value": ".grid-container", "loc": { "line": 2, "column": 1 } },
          "variables": {}
        },
        {
          "type": "StyleRule",
          "style": [
            { "name": "gap", "value": "10px",
              "keyLoc": { "line": 3, "column": 3 },
              "valLoc": { "line": 3, "column": 8 } }
          ],
          "selectorText": { "value": ".grid-item", "loc": { "line": 3, "column": 1 } },
          "variables": {}
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kSupports);

  auto* supports_rule =
      static_cast<encoder::LynxStyleRuleCondition*>(rules[0].get());
  EXPECT_EQ(supports_rule->condition, "(display:grid)");
  ASSERT_EQ(supports_rule->child_rules.size(), 2u);
  EXPECT_EQ(supports_rule->child_rules[0]->type, CSSRuleType::kStyle);
  EXPECT_EQ(supports_rule->child_rules[1]->type, CSSRuleType::kStyle);
}

TEST_F(CSSRuleParserTest, ParseConditionRuleChildWithoutTypeSkipped) {
  const char* json_str = R"json([
    {
      "type": "MediaRule",
      "prelude": { "value": "(min-width:600px)", "loc": { "line": 1, "column": 1 } },
      "rules": [
        {
          "style": [
            { "name": "color", "value": "red",
              "keyLoc": { "line": 2, "column": 1 },
              "valLoc": { "line": 2, "column": 8 } }
          ],
          "selectorText": { "value": ".a", "loc": { "line": 2, "column": 1 } },
          "variables": {}
        },
        {
          "type": "StyleRule",
          "style": [
            { "name": "color", "value": "blue",
              "keyLoc": { "line": 3, "column": 1 },
              "valLoc": { "line": 3, "column": 8 } }
          ],
          "selectorText": { "value": ".b", "loc": { "line": 3, "column": 1 } },
          "variables": {}
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);

  auto* media_rule =
      static_cast<encoder::LynxStyleRuleCondition*>(rules[0].get());
  EXPECT_EQ(media_rule->child_rules.size(), 1u);
}

TEST_F(CSSRuleParserTest, ParseStyleRuleWithCompoundSelector) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "red",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 8 } }
      ],
      "selectorText": { "value": ".a .b", "loc": { "line": 1, "column": 1 } },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);

  auto* style_rule = static_cast<encoder::LynxStyleRule*>(rules[0].get());
  EXPECT_GT(style_rule->flattened_size, 1u);
}

TEST_F(CSSRuleParserTest, ParseStyleRuleWithCommaSeparatedSelectors) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "red",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 8 } }
      ],
      "selectorText": { "value": ".a, .b", "loc": { "line": 1, "column": 1 } },
      "variables": {}
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);

  auto* style_rule = static_cast<encoder::LynxStyleRule*>(rules[0].get());
  EXPECT_GT(style_rule->flattened_size, 1u);
}

TEST_F(CSSRuleParserTest, ParseKeyframesRuleWithSingleKeyframe) {
  const char* json_str = R"json([
    {
      "type": "KeyframesRule",
      "name": { "value": "slide", "loc": { "line": 1, "column": 1 } },
      "styles": [
        {
          "keyText": { "value": "from", "loc": { "line": 1, "column": 1 } },
          "variables": {},
          "style": [
            { "name": "transform", "value": "translateX(0)",
              "keyLoc": { "line": 1, "column": 10 },
              "valLoc": { "line": 1, "column": 21 } }
          ]
        }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kKeyframes);

  auto* keyframes_rule =
      static_cast<encoder::LynxStyleRuleKeyframes*>(rules[0].get());
  EXPECT_EQ(keyframes_rule->name, "slide");
  EXPECT_NE(keyframes_rule->properties, nullptr);
}

TEST_F(CSSRuleParserTest, ParseFontFaceRuleWithOnlyFamily) {
  const char* json_str = R"json([
    {
      "type": "FontFaceRule",
      "style": [
        { "name": "font-family", "value": "MyFont",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 14 } }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  ASSERT_EQ(rules.size(), 1u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kFontFace);

  auto* fontface_rule =
      static_cast<encoder::LynxStyleRuleFontFace*>(rules[0].get());
  EXPECT_EQ(fontface_rule->family, "MyFont");
  EXPECT_EQ(fontface_rule->properties.size(), 1u);
}

TEST_F(CSSRuleParserTest, ParseMixedRulesWithInvalidEntriesFiltered) {
  const char* json_str = R"json([
    {
      "type": "StyleRule",
      "style": [
        { "name": "color", "value": "red",
          "keyLoc": { "line": 1, "column": 1 },
          "valLoc": { "line": 1, "column": 8 } }
      ],
      "selectorText": { "value": ".valid", "loc": { "line": 1, "column": 1 } },
      "variables": {}
    },
    {
      "type": "StyleRule",
      "variables": {}
    },
    {
      "type": "UnknownRule"
    },
    {
      "no_type_field": true
    },
    {
      "type": "FontFaceRule",
      "style": [
        { "name": "font-family", "value": "ValidFont",
          "keyLoc": { "line": 5, "column": 1 },
          "valLoc": { "line": 5, "column": 14 } }
      ]
    }
  ])json";

  auto json = base::strToJson(json_str);
  CSSRuleParser impl(compile_options_);
  auto fragment = impl.ParseCSSRules(json, "/test.ttss", {}, 0);

  ASSERT_NE(fragment, nullptr);
  const auto& rules = fragment->rules();
  EXPECT_EQ(rules.size(), 2u);
  EXPECT_EQ(rules[0]->type, CSSRuleType::kStyle);
  EXPECT_EQ(rules[1]->type, CSSRuleType::kFontFace);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
