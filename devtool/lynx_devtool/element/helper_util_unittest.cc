// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/helper_util.h"

#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

TEST(HelperUtilTest, ConvertLepusValueToJsonValueTest) {
  lepus::Value value1("bar");
  EXPECT_EQ(ConvertLepusValueToJsonValue(value1), "\"bar\"");

  lepus::Value value2(0);
  EXPECT_EQ(ConvertLepusValueToJsonValue(value2), "0");

  lepus::Value value3(true);
  EXPECT_EQ(ConvertLepusValueToJsonValue(value3), "1");

  lepus::Value value4(3.1415926);
  EXPECT_EQ(ConvertLepusValueToJsonValue(value4), "3.141593");

  lepus::Value value5(33333);
  EXPECT_EQ(ConvertLepusValueToJsonValue(value5), "33333");

  int64_t int64_value = 1234567890123456789;
  lepus::Value value6(int64_value);
  EXPECT_EQ(ConvertLepusValueToJsonValue(value6), "1234567890123456789");

  uint32_t a = 2147483648;
  lepus::Value value7(a);
  EXPECT_EQ(ConvertLepusValueToJsonValue(value7), "2147483648");

  uint64_t b = 1234567890123456789;
  lepus::Value value8(b);
  EXPECT_EQ(ConvertLepusValueToJsonValue(value8), "1234567890123456789");

  lepus::Value value9(lepus::Dictionary::Create());
  value9.SetProperty("str", lepus::Value("bar"));
  value9.SetProperty("num", lepus::Value(0xff));
  value9.SetProperty("bool", lepus::Value(false));
  value9.SetProperty("null", lepus::Value());
  EXPECT_EQ(ConvertLepusValueToJsonValue(value9),
            "{\"null\": null,\"bool\": 0,\"num\": 255,\"str\": \"bar\"}");

  lepus::Value dict1 = lepus::Value(lepus::Dictionary::Create());
  lepus::Value dict2 = lepus::Value(lepus::Dictionary::Create());
  lepus::Value dict3 = lepus::Value(lepus::Dictionary::Create());
  dict1.SetProperty("c", lepus::Value(true));
  dict1.SetProperty("d", lepus::Value("url_link"));
  dict1.SetProperty("e", lepus::Value(3333335));
  dict2.SetProperty("a", lepus::Value());
  dict2.SetProperty("b", lepus::Value(3.89756));
  dict2.SetProperty("f", dict1);
  dict2.SetProperty("g", dict3);
  EXPECT_EQ(ConvertLepusValueToJsonValue(dict2),
            "{\"g\": {},\"b\": 3.897560,\"f\": {\"e\": 3333335,\"d\": "
            "\"url_link\",\"c\": 1},\"a\": null}");
}

TEST(HelperUtilTest, NormalizeAnimationStringTest) {
  std::string value = "";

  value =
      R"({"65": "animation_name1", "66": 5005, "name": "Alice", "68": 903, "69": 55555, "70": 0, "71": 2, "72": 1})";
  EXPECT_EQ(lynx::devtool::NormalizeAnimationString(value),
            "animation_name1 5.005s 903ms 55555 normal backwards running");

  value = R"([{"65": "animation_name2", "70": 2, "71": 0, "72": 0}])";
  EXPECT_EQ(lynx::devtool::NormalizeAnimationString(value),
            "animation_name2 alternate none paused");

  value = R"([{"65": "animation_name3", "70": 1, "71": 3}])";
  EXPECT_EQ(lynx::devtool::NormalizeAnimationString(value),
            "animation_name3 reverse both");
}

TEST(HelperUtilTest, GetAnimationNamesTest) {
  std::string value = "test_value";
  std::vector<std::string> animation_names;

  animation_names.push_back(value);
  EXPECT_EQ(animation_names, lynx::devtool::GetAnimationNames(value, false));

  value = R"({"name": "display", "value": "linear", "65": "animation_name1"})";
  animation_names.clear();
  animation_names.push_back("animation_name1");
  EXPECT_EQ(lynx::devtool::GetAnimationNames(value, true), animation_names);

  value = R"({"name": "display", "value": "linear"})";
  EXPECT_EQ(lynx::devtool::GetAnimationNames(value, true).size(), 0);

  value =
      R"([{"name": "John", "age": 30}, {"name": "display", "65": "animation_name2"}])";
  animation_names.clear();
  animation_names.push_back("animation_name2");
  EXPECT_EQ(lynx::devtool::GetAnimationNames(value, true), animation_names);

  value = R"([{"name": "John", "age": 30}, {"name": "display", "age": "65"}])";
  EXPECT_EQ(lynx::devtool::GetAnimationNames(value, true).size(), 0);
}

TEST(HelperUtilTest, MergeCSSStyleTEST) {
  Json::Value res1, res2;
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
                          {27, 10, 12, 10}}},
                        {"linear-layout-gravity-string",
                         {"linear-layout-gravity",
                          "center-vertical",
                          "linear-layout-gravity:center-vertical;",
                          false,
                          false,
                          false,
                          false,
                          true,
                          {65, 10, 27, 10}}},
                        {"font-size",
                         {"font-size",
                          "32rpx",
                          "font-size:32rpx;",
                          false,
                          false,
                          false,
                          false,
                          true,
                          {81, 10, 65, 10}}}};
  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.origin_ = "regular";
  style_sheet.css_text_ = ".label_text";
  style_sheet.css_properties_ = css_properties;
  style_sheet.style_value_range_ = {81, 10, 12, 10};

  lynx::devtool::MergeCSSStyle(res1, style_sheet, true);
  EXPECT_EQ(res1, res2);
}

TEST(HelperUtilTest, ReplaceDefaultComputedStyleTest) {
  std::unordered_map<std::string, std::string> dict1, dict2;
  std::unordered_multimap<std::string, lynx::devtool::CSSPropertyDetail>
      css_attrs_map = {
          {"new-color",
           {"color", "red", "text1", false, true, true, false, true}},
          {"new-font-size",
           {"font-size", "16px", "text2", true, true, true, false, true}},
          {"new-background-color",
           {"background-color", "white", "text3", false, true, true, false,
            false}}};
  dict1.insert({"new-color", "red"});

  lynx::devtool::ReplaceDefaultComputedStyle(dict2, css_attrs_map);
  EXPECT_EQ(dict1, dict2);
}

TEST(HelperUtilTest, StripSpaceTest) {
  std::string str = " \n A\t b  \r\n\t";
  EXPECT_EQ(lynx::devtool::StripSpace(str), "A\t b");

  str = "    \n\t\r    ";
  EXPECT_EQ(lynx::devtool::StripSpace(str), "");

  std::string str1 = "12345";
  EXPECT_EQ(lynx::devtool::StripSpace(str1), "12345");
  str1 = " 12345";
  EXPECT_EQ(lynx::devtool::StripSpace(str1), "12345");
  str1 = "12345 ";
  EXPECT_EQ(lynx::devtool::StripSpace(str1), "12345");
  str1 = " 12345 ";
  EXPECT_EQ(lynx::devtool::StripSpace(str1), "12345");
  str1 = "";
  EXPECT_EQ(lynx::devtool::StripSpace(str1), "");
  str1 = "     ";
  EXPECT_EQ(lynx::devtool::StripSpace(str1), "");
  str1 = R"(
    12345
  )";
  EXPECT_EQ(lynx::devtool::StripSpace(str1), "12345");
}

TEST(HelperUtilTest, IsSpaceTest) {
  char c = 'A';
  EXPECT_FALSE(lynx::devtool::IsSpace(c));
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
