// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/helper_util.h"

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
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
  EXPECT_EQ(ConvertLepusValueToJsonValue(value9, true),
            "{\"bool\": 0,\"null\": null,\"num\": 255,\"str\": \"bar\"}");

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
  EXPECT_EQ(ConvertLepusValueToJsonValue(dict2, true),
            "{\"a\": null,\"b\": 3.897560,\"f\": {\"c\": 1,\"d\": "
            "\"url_link\",\"e\": 3333335},\"g\": {}}");

  lepus::Value array1(lepus::CArray::Create());
  array1.SetProperty(0, lepus::Value("foo"));
  array1.SetProperty(1, lepus::Value(42));
  lepus::Value array_item(lepus::Dictionary::Create());
  array_item.SetProperty("ok", lepus::Value(true));
  array1.SetProperty(2, array_item);
  EXPECT_EQ(ConvertLepusValueToJsonValue(array1, true),
            "[\"foo\",42,{\"ok\": 1}]");
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

TEST(HelperUtilTest, MergeCSSStyleSerializesMediaRule) {
  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.empty = false;
  style_sheet.origin_ = "regular";
  style_sheet.style_sheet_id_ = "12";
  style_sheet.style_name_ = ".card";
  style_sheet.css_text_ = "width:10px;";
  style_sheet.style_name_range_ = {1, 1, 0, 5};
  style_sheet.style_value_range_ = {1, 1, 6, 17};
  style_sheet.media_text_ = "(min-width: 1000px)";
  style_sheet.media_range_ = {0, 0, 0, 19};
  style_sheet.property_order_.push_back("width");
  style_sheet.css_properties_.insert({"width",
                                      {"width",
                                       "10px",
                                       "width:10px;",
                                       false,
                                       false,
                                       false,
                                       false,
                                       true,
                                       {1, 1, 6, 17}}});

  Json::Value result(Json::ValueType::arrayValue);
  lynx::devtool::MergeCSSStyle(result, style_sheet, true);

  ASSERT_TRUE(result.isArray());
  ASSERT_EQ(result.size(), 1U);
  const Json::Value& media = result[0]["rule"]["media"];
  ASSERT_TRUE(media.isArray());
  ASSERT_EQ(media.size(), 1U);
  EXPECT_EQ(media[0]["text"].asString(), "(min-width: 1000px)");
  EXPECT_EQ(media[0]["source"].asString(), "mediaRule");
  EXPECT_EQ(media[0]["styleSheetId"].asString(), "12");
  EXPECT_EQ(media[0]["range"]["startLine"].asInt(), 0);
  EXPECT_EQ(media[0]["range"]["endLine"].asInt(), 0);
  EXPECT_EQ(media[0]["range"]["startColumn"].asInt(), 0);
  EXPECT_EQ(media[0]["range"]["endColumn"].asInt(), 19);
}

TEST(HelperUtilTest, MergeCSSStyleSerializesCascadeLayersOuterToInner) {
  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.empty = false;
  style_sheet.origin_ = "regular";
  style_sheet.style_sheet_id_ = "12";
  style_sheet.style_name_ = ".card";
  style_sheet.css_text_ = "width:10px;";
  style_sheet.style_name_range_ = {1, 1, 0, 5};
  style_sheet.style_value_range_ = {1, 1, 6, 17};
  style_sheet.layers_ = {"framework", "components"};

  Json::Value result(Json::ValueType::arrayValue);
  lynx::devtool::MergeCSSStyle(result, style_sheet, true);

  ASSERT_TRUE(result.isArray());
  ASSERT_EQ(result.size(), 1U);
  const Json::Value& layers = result[0]["rule"]["layers"];
  ASSERT_TRUE(layers.isArray());
  ASSERT_EQ(layers.size(), 2U);
  EXPECT_EQ(layers[0]["text"].asString(), "framework");
  EXPECT_EQ(layers[1]["text"].asString(), "components");
}

TEST(HelperUtilTest, MergeCSSStyleOmitsEmptyCascadeLayers) {
  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.empty = false;
  style_sheet.origin_ = "regular";
  style_sheet.style_name_ = ".card";

  Json::Value result(Json::ValueType::arrayValue);
  lynx::devtool::MergeCSSStyle(result, style_sheet, true);

  ASSERT_EQ(result.size(), 1U);
  EXPECT_FALSE(result[0]["rule"].isMember("layers"));
}

TEST(HelperUtilTest, MergeCSSStyleSerializesAnonymousCascadeLayer) {
  lynx::devtool::InspectorStyleSheet style_sheet;
  style_sheet.empty = false;
  style_sheet.origin_ = "regular";
  style_sheet.style_name_ = ".card";
  style_sheet.layers_ = {"framework", ""};

  Json::Value result(Json::ValueType::arrayValue);
  lynx::devtool::MergeCSSStyle(result, style_sheet, true);

  const Json::Value& layers = result[0]["rule"]["layers"];
  ASSERT_EQ(layers.size(), 2U);
  EXPECT_EQ(layers[0]["text"].asString(), "framework");
  EXPECT_EQ(layers[1]["text"].asString(), "");
}

TEST(HelperUtilTest, StyleTextParserPreservesMediaInfo) {
  lynx::devtool::InspectorStyleSheet pre_style;
  pre_style.empty = false;
  pre_style.origin_ = "regular";
  pre_style.style_sheet_id_ = "12";
  pre_style.style_name_ = ".card";
  pre_style.style_name_range_ = {1, 1, 0, 5};
  pre_style.media_text_ = "(min-width: 1000px)";
  pre_style.media_range_ = {0, 0, 0, 19};

  auto parsed = lynx::devtool::StyleTextParser<lynx::tasm::Element*>(
      nullptr, "width:20px", pre_style);

  EXPECT_EQ(parsed.media_text_, "(min-width: 1000px)");
  EXPECT_EQ(parsed.media_range_.start_line_, 0);
  EXPECT_EQ(parsed.media_range_.end_line_, 0);
  EXPECT_EQ(parsed.media_range_.start_column_, 0);
  EXPECT_EQ(parsed.media_range_.end_column_, 19);
}

TEST(HelperUtilTest, StyleTextParserPreservesCascadeLayers) {
  lynx::devtool::InspectorStyleSheet pre_style;
  pre_style.empty = false;
  pre_style.origin_ = "regular";
  pre_style.style_sheet_id_ = "12";
  pre_style.style_name_ = ".card";
  pre_style.style_name_range_ = {1, 1, 0, 5};
  pre_style.layers_ = {"framework", "components"};

  auto parsed = lynx::devtool::StyleTextParser<lynx::tasm::Element*>(
      nullptr, "width:20px", pre_style);

  const std::vector<std::string> expected{"framework", "components"};
  EXPECT_EQ(expected, parsed.layers_);
}

TEST(HelperUtilTest, StyleTextParserExtractsImportantAnnotation) {
  lynx::devtool::InspectorStyleSheet pre_style;
  pre_style.empty = false;
  pre_style.origin_ = "regular";
  pre_style.style_sheet_id_ = "12";
  pre_style.style_name_ = ".card";
  pre_style.style_name_range_ = {1, 1, 0, 5};

  auto parsed = lynx::devtool::StyleTextParser<lynx::tasm::Element*>(
      nullptr, "width:20px !important;", pre_style);

  auto property = parsed.css_properties_.find("width");
  ASSERT_NE(property, parsed.css_properties_.end());
  EXPECT_EQ(property->second.value_, "20px");
  EXPECT_EQ(property->second.text_, "width:20px !important;");
  EXPECT_TRUE(property->second.important_);

  Json::Value result(Json::ValueType::arrayValue);
  lynx::devtool::MergeCSSStyle(result, parsed, true);
  const Json::Value& serialized =
      result[0]["rule"]["style"]["cssProperties"][0];
  EXPECT_EQ(serialized["value"].asString(), "20px !important");
  EXPECT_TRUE(serialized["important"].asBool());
}

TEST(HelperUtilTest, CSSPropertyValueForProtocolAppendsImportantOnce) {
  EXPECT_EQ(lynx::devtool::CSSPropertyValueForProtocol("20px", true),
            "20px !important");
  EXPECT_EQ(lynx::devtool::CSSPropertyValueForProtocol("20px !important", true),
            "20px !important");
  EXPECT_EQ(lynx::devtool::CSSPropertyValueForProtocol("20px", false), "20px");
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
