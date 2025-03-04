// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_variable_handler.h"

#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class CSSVariableHandlerTest : public ::testing::Test {
 public:
  CSSVariableHandlerTest() {}
  ~CSSVariableHandlerTest() override {}

  void SetUp() override {}
};

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

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
