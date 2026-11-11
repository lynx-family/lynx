// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_variable_handler.h"

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/style_resolver.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static_assert(sizeof(StyleResolver) == 1,
              "StyleResolver must remain stateless to fit Element's padding");

class CSSVariableHandlerTest : public ::testing::Test {
 public:
  CSSVariableHandlerTest() {}
  ~CSSVariableHandlerTest() override {}

  void SetUp() override {}
};

namespace {
CSSValue ParseCacheExpression(const char* expression) {
  CSSParserConfigs configs;
  lepus::Value value(expression);
  CSSStringParser parser = CSSStringParser::FromLepusString(value, configs);
  return parser.ParseVariable();
}
}  // namespace

TEST_F(CSSVariableHandlerTest, CachedSubstitutionInvalidatesOnlyUsedInputs) {
  CSSVariableHandler::ResolutionCache cache;
  CSSVariableHandler handler(true, &cache);
  CSSParserConfigs configs;
  CustomPropertiesMap properties;
  properties.insert_or_assign("--width", ParseCacheExpression("12px"));
  const auto expression = ParseCacheExpression("var(--width) 0px");
  StyleMap result;
  handler.ResolveCSSVariables(kPropertyIDPadding, expression, result,
                              &properties, configs);
  ASSERT_EQ(result[kPropertyIDPaddingTop].AsNumber(), 12);
  const auto cached_string =
      cache.entries.find(expression.AsString())->second.result;

  properties.insert_or_assign("--unrelated", ParseCacheExpression("24px"));
  result.clear();
  handler.ResolveCSSVariables(kPropertyIDPadding, expression, result,
                              &properties, configs);
  EXPECT_EQ(result[kPropertyIDPaddingTop].AsNumber(), 12);
  EXPECT_EQ(base::String::Unsafe::GetStringRawRef(
                cache.entries.find(expression.AsString())->second.result),
            base::String::Unsafe::GetStringRawRef(cached_string));

  properties.insert_or_assign("--width", ParseCacheExpression("32px"));
  result.clear();
  handler.ResolveCSSVariables(kPropertyIDPadding, expression, result,
                              &properties, configs);
  EXPECT_EQ(result[kPropertyIDPaddingTop].AsNumber(), 32);
}

TEST_F(CSSVariableHandlerTest, CachedSubstitutionTracksMissingFallbackInputs) {
  CSSVariableHandler::ResolutionCache cache;
  CSSVariableHandler handler(true, &cache);
  CSSParserConfigs configs;
  CustomPropertiesMap properties;
  const auto expression = ParseCacheExpression("var(--missing,7px)");
  StyleMap result;
  handler.ResolveCSSVariables(kPropertyIDWidth, expression, result, &properties,
                              configs);
  ASSERT_EQ(result[kPropertyIDWidth].AsNumber(), 7);

  properties.insert_or_assign("--missing", ParseCacheExpression("24px"));
  result.clear();
  handler.ResolveCSSVariables(kPropertyIDWidth, expression, result, &properties,
                              configs);
  EXPECT_EQ(result[kPropertyIDWidth].AsNumber(), 24);
  properties.erase("--missing");
  result.clear();
  handler.ResolveCSSVariables(kPropertyIDWidth, expression, result, &properties,
                              configs);
  EXPECT_EQ(result[kPropertyIDWidth].AsNumber(), 7);
}

TEST_F(CSSVariableHandlerTest,
       CachedSubstitutionInvalidatesChangedLegacyFallback) {
  CSSVariableHandler::ResolutionCache cache;
  CSSVariableHandler handler(true, &cache);
  CSSParserConfigs configs;
  for (const bool normalize : {false, true}) {
    for (const char* fallback : {"12px", "24px"}) {
      CSSValue expression("{{--missing}}", CSSValuePattern::STRING,
                          CSSValueType::VARIABLE);
      expression.SetDefaultValue(fallback);
      if (normalize) {
        expression.ToVarReference();
      }
      StyleMap result;
      handler.ResolveCSSVariables(kPropertyIDWidth, expression, result, nullptr,
                                  configs);
      EXPECT_EQ(result[kPropertyIDWidth].AsNumber(),
                fallback == std::string("12px") ? 12 : 24);
    }
  }
}

TEST_F(CSSVariableHandlerTest, CachedSubstitutionRefreshesTransitiveBindings) {
  CSSVariableHandler::ResolutionCache cache;
  CSSVariableHandler handler(true, &cache);
  CSSParserConfigs configs;
  CustomPropertiesMapRef specified;
  specified.Init();
  auto& properties = specified.Access()->Value();
  properties.insert_or_assign("--left", ParseCacheExpression("12px"));
  properties.insert_or_assign("--right", ParseCacheExpression("12px"));
  properties.insert_or_assign("--alias", ParseCacheExpression("var(--left)"));
  CustomPropertiesMapRef resolved;
  CSSValue::CustomPropertyResolutionCacheRef property_cache;
  CSSValue::ResolveCustomProperties(specified, resolved, property_cache);

  const auto expression = ParseCacheExpression("var(--alias) 0px");
  CSSVariableMap dependencies;
  const auto record = [&dependencies](const base::String& name,
                                      const base::String& value) {
    dependencies.insert_or_assign(name, value);
  };
  StyleMap result;
  handler.ResolveCSSVariables(kPropertyIDPadding, expression, result,
                              &resolved->Value(), configs, record);
  ASSERT_TRUE(dependencies.contains("--left"));
  auto& cached = cache.entries.find(expression.AsString())->second;
  ASSERT_EQ(cached.inputs.size(), 1U);
  const auto cached_string = cached.result;

  specified.Access()->Value().insert_or_assign(
      "--alias", ParseCacheExpression("var(--right)"));
  CSSValue::ResolveCustomProperties(specified, resolved, property_cache);
  dependencies.clear();
  result.clear();
  handler.ResolveCSSVariables(kPropertyIDPadding, expression, result,
                              &resolved->Value(), configs, record);
  EXPECT_EQ(result[kPropertyIDPaddingTop].AsNumber(), 12);
  EXPECT_EQ(base::String::Unsafe::GetStringRawRef(cached.result),
            base::String::Unsafe::GetStringRawRef(cached_string));
  EXPECT_TRUE(dependencies.contains("--right"));
  EXPECT_FALSE(dependencies.contains("--left"));
}

TEST_F(CSSVariableHandlerTest, DirectReferenceTracksBindingsAndStripsPriority) {
  CSSParserConfigs configs;
  const auto expression = ParseCacheExpression("var(--width,var(--fallback))");
  for (const bool enable_cache : {false, true}) {
    CSSVariableHandler::ResolutionCache cache;
    CSSVariableHandler handler(true, enable_cache ? &cache : nullptr);
    CustomPropertiesMap properties;
    properties.insert_or_assign("--width",
                                ParseCacheExpression("12px !important"));
    properties.insert_or_assign("--fallback", ParseCacheExpression("24px"));
    CSSVariableMap dependencies;
    const auto record = [&dependencies](const base::String& name,
                                        const base::String& value) {
      dependencies.insert_or_assign(name, value);
    };
    StyleMap result;
    handler.ResolveCSSVariables(kPropertyIDWidth, expression, result,
                                &properties, configs, record);
    EXPECT_EQ(result[kPropertyIDWidth].AsNumber(), 12);
    EXPECT_EQ(dependencies["--width"], "12px !important");
    EXPECT_FALSE(dependencies.contains("--fallback"));

    properties.erase("--width");
    result.clear();
    dependencies.clear();
    handler.ResolveCSSVariables(kPropertyIDWidth, expression, result,
                                &properties, configs, record);
    EXPECT_EQ(result[kPropertyIDWidth].AsNumber(), 24);
    EXPECT_TRUE(dependencies.contains("--fallback"));

    properties.insert_or_assign("--width", ParseCacheExpression("32px"));
    result.clear();
    dependencies.clear();
    handler.ResolveCSSVariables(kPropertyIDWidth, expression, result,
                                &properties, configs, record);
    EXPECT_EQ(result[kPropertyIDWidth].AsNumber(), 32);
    EXPECT_FALSE(dependencies.contains("--fallback"));
  }
}

TEST_F(CSSVariableHandlerTest, LongCustomPropertyChainRemainsResolvable) {
  starlight::ComputedCSSStyle style{1.f, 1.f};
  for (int i = 0; i < 16; ++i) {
    const auto name = "--var" + std::to_string(i);
    const auto expression =
        i == 15 ? "red" : "var(--var" + std::to_string(i + 1) + ")";
    style.SetCustomProperty(base::String(name),
                            ParseCacheExpression(expression.c_str()));
  }
  style.FinalizeCustomProperties();
  EXPECT_EQ(style.GetCustomProperties()->find("--var0")->second.AsString(),
            "red");
}

TEST_F(CSSVariableHandlerTest, CachedSubstitutionKeepsAncestorBindings) {
  CSSVariableHandler::ResolutionCache cache;
  CSSVariableHandler handler(true, &cache);
  CSSParserConfigs configs;
  starlight::ComputedCSSStyle parent{1.f, 1.f};
  parent.SetCustomProperty("--base", ParseCacheExpression("12px"));
  parent.SetCustomProperty("--alias", ParseCacheExpression("var(--base)"));
  parent.FinalizeCustomProperties();
  starlight::ComputedCSSStyle child{1.f, 1.f};
  child.InheritCustomPropertiesFrom(parent);
  child.SetCustomProperty("--base", ParseCacheExpression("24px"));
  child.FinalizeCustomProperties();

  for (const auto& [source, expected_right] :
       {std::make_pair("var(--alias)", 12),
        std::make_pair("var(--alias) var(--base) var(--alias)", 24)}) {
    const auto expression = ParseCacheExpression(source);
    for (int i = 0; i < 2; ++i) {
      CSSVariableMap dependencies;
      StyleMap result;
      handler.ResolveCSSVariables(
          kPropertyIDPadding, expression, result, child.GetCustomProperties(),
          configs,
          [&dependencies](const base::String& name, const base::String& value) {
            dependencies.insert_or_assign(name, value);
          });
      EXPECT_EQ(result[kPropertyIDPaddingTop].AsNumber(), 12);
      EXPECT_EQ(result[kPropertyIDPaddingRight].AsNumber(), expected_right);
      EXPECT_EQ(result[kPropertyIDPaddingBottom].AsNumber(), 12);
      EXPECT_EQ(dependencies["--base"], "12px");
    }
  }
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule0) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--main-bg-color", "red");
  base::String default_props;
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule("{{--main-bg-color}}",
                                node.attribute_holder().get(), default_props,
                                default_value_map)
          .c_str();
  ASSERT_EQ(result, "red");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule1) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--height", "20px");
  base::String default_props;
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule("calc({{--height}} + {{--height}})",
                                node.attribute_holder().get(), default_props,
                                default_value_map)
          .c_str();
  ASSERT_EQ(result, "calc(20px + 20px)");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule2) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--height", "20px");
  base::String default_props;
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule("calc({{--height}} - {{--height}})",
                                node.attribute_holder().get(), default_props,
                                default_value_map)
          .c_str();
  ASSERT_EQ(result, "calc(20px - 20px)");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule3) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--height", "20px");
  base::String default_props;
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule("calc({{--height}} * {{--height}})",
                                node.attribute_holder().get(), default_props,
                                default_value_map)
          .c_str();
  ASSERT_EQ(result, "calc(20px * 20px)");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule4) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--height", "20px");
  base::String default_props;
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule("calc({{--height}} / {{--height}})",
                                node.attribute_holder().get(), default_props,
                                default_value_map)
          .c_str();
  ASSERT_EQ(result, "calc(20px / 20px)");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule5) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--height", "20px");
  node.UpdateCSSVariable("--width", "40px");
  base::String default_props;
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule("calc({{--height}} + {{--width}})",
                                node.attribute_holder().get(), default_props,
                                default_value_map)
          .c_str();
  ASSERT_EQ(result, "calc(20px + 40px)");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule6) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--height", "20px");
  node.UpdateCSSVariable("--width", "40px");
  base::String default_props;
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule(
              "calc({{--height}} + {{--width}} * {{--height}})",
              node.attribute_holder().get(), default_props, default_value_map)
          .c_str();
  ASSERT_EQ(result, "calc(20px + 40px * 20px)");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule7) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--height", "20px");
  node.UpdateCSSVariable("--width", "40px");
  base::String default_props = "calc(300px - 100px)";
  lepus::Value default_value_map;
  std::string result =
      handler
          .GetCSSVariableByRule("calc({{--main-height}} - 100px)",
                                node.attribute_holder().get(), default_props,
                                default_value_map)
          .c_str();
  ASSERT_EQ(result, "calc(300px - 100px)");
}

TEST_F(CSSVariableHandlerTest, FormatStringWithRule8) {
  CSSVariableHandler handler;
  RadonNode node(nullptr, "my_tag", 0);
  node.UpdateCSSVariable("--var-style", "solid");
  //   node.UpdateCSSVariable("--var-color", "black");
  base::String default_props = "2px double red";
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("--var-style"), lepus::Value("double"));
  table->SetValue(base::String("--var-color"), lepus::Value("red"));
  lepus::Value default_value_map = lepus::Value(table);
  std::string result;
  result = handler
               .GetCSSVariableByRule("2px {{--var-style}} {{--var-color}}",
                                     node.attribute_holder().get(),
                                     default_props, default_value_map)
               .c_str();
  ASSERT_EQ(result, "2px solid red");
}

TEST_F(CSSVariableHandlerTest, HasCSSVariableInAnyStyleMap) {
  CSSVariableHandler handler;

  StyleMap map_with_var;
  CSSValue var_value("{{--test}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  var_value.ToVarReference();
  map_with_var.insert_or_assign(kPropertyIDTop, var_value);

  StyleMap map_without_var;
  map_without_var.insert_or_assign(kPropertyIDLeft,
                                   CSSValue(10.0f, CSSValuePattern::NUMBER));

  // True when at least one map contains a variable.
  EXPECT_TRUE(
      handler.HasCSSVariableInAnyStyleMap({&map_without_var, &map_with_var}));

  // True on the first map that contains a variable.
  EXPECT_TRUE(
      handler.HasCSSVariableInAnyStyleMap({&map_with_var, &map_without_var}));

  // False when no maps contain variables.
  EXPECT_FALSE(handler.HasCSSVariableInAnyStyleMap(
      {&map_without_var, &map_without_var}));

  // Null pointers are skipped.
  EXPECT_TRUE(handler.HasCSSVariableInAnyStyleMap({nullptr, &map_with_var}));
  EXPECT_FALSE(
      handler.HasCSSVariableInAnyStyleMap({nullptr, &map_without_var}));
}

TEST_F(CSSVariableHandlerTest, ResolveCSSVariables) {
  CSSVariableHandler handler(true);

  CustomPropertiesMap custom_props;
  custom_props.insert_or_assign(
      base::String("--test"),
      CSSValue(lepus::Value("20px"), CSSValuePattern::STRING));

  CSSValue var_value("{{--test}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  var_value.ToVarReference();

  StyleMap result;
  handler.ResolveCSSVariables(kPropertyIDTop, var_value, result, &custom_props,
                              CSSParserConfigs());

  auto it = result.find(kPropertyIDTop);
  ASSERT_NE(it, result.end());
  // UnitHandler::Process should parse "20px" as a length value.
  EXPECT_TRUE(it->second.IsPx());
  EXPECT_EQ(it->second.GetNumber(), 20);
}

TEST_F(CSSVariableHandlerTest, ResolveCSSVariablesNullProps) {
  CSSVariableHandler handler(true);

  CSSValue var_value("{{--missing}}", CSSValuePattern::STRING,
                     CSSValueType::VARIABLE);
  var_value.ToVarReference();

  StyleMap result;
  // Passing nullptr custom_properties should not crash (falls back to empty).
  handler.ResolveCSSVariables(kPropertyIDTop, var_value, result, nullptr,
                              CSSParserConfigs());

  // Result is empty because the variable cannot be resolved.
  EXPECT_TRUE(result.empty());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
